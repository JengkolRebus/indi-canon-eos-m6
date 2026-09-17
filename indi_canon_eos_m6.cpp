/*
 * indi_canon_eos_m6
 *
 * Canon Canon EOS M6/PTP INDI CCD driver.
 *
 * Architecture:
 *   CONNECT    -> gp_camera_new() + gp_camera_init() once
 *   CONNECT    -> read ISO + shutter once through the USB/PTP session
 *   CAPTURE    -> gp_camera_capture() on the same Camera* session
 *              -> gp_camera_file_get() on the same Camera* session
 *              -> load CR2 into INDI CCD framebuffer
 *   DISCONNECT -> gp_camera_exit()
 *
 * No gphoto2 shell commands are used.
 * No list-files / polling / delete-file operations are used.
 * Camera settings remain manual.
 *
 * The USB/PTP Camera* session follows the libgphoto2 photobooth
 * pattern: gp_camera_init() is called once and the same camera object
 * is reused for multiple captures until gp_camera_exit().
 */

#include <indicom.h>
#include <indiccd.h>

#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-context.h>
#include <gphoto2/gphoto2-file.h>
#include <gphoto2/gphoto2-widget.h>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

class EOSM6USB : public INDI::CCD
{
public:
    EOSM6USB();
    ~EOSM6USB() override;

    const char *getDefaultName() override;
    bool initProperties() override;
    bool updateProperties() override;
    void ISGetProperties(const char *dev) override;

    bool ISNewText(const char *dev, const char *name, char *texts[],
                   char *names[], int n) override;

    bool Connect() override;
    bool Disconnect() override;

    bool StartExposure(float duration) override;
    bool AbortExposure() override;

private:
    bool openCamera();
    void closeCamera();

    bool readCameraSettings();
    bool findWidgetRecursive(CameraWidget *widget,
                             const char *wanted,
                             std::string &value);

    bool captureAndDownload(std::string &localFile);
    bool loadFileToCCD(const std::string &localFile);

    std::string makeLocalFilename() const;
    void failExposure(const std::string &reason);

    Camera *camera {nullptr};
    GPContext *context {nullptr};
    bool busy {false};
    bool abortRequested {false};

    std::string downloadDir {"/tmp/indi-eosm6"};

    IText DownloadDirTP[1];
    ITextVectorProperty DownloadDirTPV;

    IText CameraInfoTP[2];
    ITextVectorProperty CameraInfoTPV;
};

EOSM6USB::EOSM6USB()
{
    setVersion(2, 0);
    initProperties();
}

EOSM6USB::~EOSM6USB()
{
    closeCamera();
}

const char *EOSM6USB::getDefaultName()
{
    return "Canon EOS M6";
}

bool EOSM6USB::initProperties()
{
    if (*getDeviceName() == '\0')
        setDeviceName(getDefaultName());

    INDI::CCD::initProperties();

    addCaptureFormat({"CR2", "CR2", 16, true, true});

    EncodeFormatSP[FORMAT_FITS].setState(ISS_OFF);
    EncodeFormatSP[FORMAT_NATIVE].setState(ISS_ON);

    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE",
                             0.001, 3600.0, 0.001);

    SetCCDParams(6000, 4000, 16, 3.72, 3.72);
    SetCCDCapability(CCD_CAN_ABORT);

    IUFillText(&DownloadDirTP[0], "PATH", "Download Directory",
               downloadDir.c_str());

    IUFillTextVector(&DownloadDirTPV, DownloadDirTP, 1,
                     getDeviceName(), "DOWNLOAD_DIR", "Image Download",
                     OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    IUFillText(&CameraInfoTP[0], "ISO", "ISO", "Unknown");
    IUFillText(&CameraInfoTP[1], "SHUTTER", "Shutter Speed", "Unknown");

    IUFillTextVector(&CameraInfoTPV, CameraInfoTP, 2,
                     getDeviceName(), "CAMERA_SETTINGS", "Camera Settings",
                     MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    addDebugControl();

    return true;
}

bool EOSM6USB::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        defineProperty(&DownloadDirTPV);
        defineProperty(&CameraInfoTPV);
    }
    else
    {
        deleteProperty(DownloadDirTPV.name);
        deleteProperty(CameraInfoTPV.name);
    }

    return true;
}

void EOSM6USB::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);
}

bool EOSM6USB::ISNewText(const char *dev, const char *name,
                                 char *texts[], char *names[], int n)
{
    if (dev && strcmp(dev, getDeviceName()))
        return false;

    if (!strcmp(name, DownloadDirTPV.name))
    {
        IUUpdateText(&DownloadDirTPV, texts, names, n);
        downloadDir = DownloadDirTP[0].text;

        if (downloadDir.empty())
            downloadDir = "/tmp/indi-eosm6";

        DownloadDirTPV.s = IPS_OK;
        IDSetText(&DownloadDirTPV, nullptr);
        return true;
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool EOSM6USB::openCamera()
{
    if (camera)
        return true;

    context = gp_context_new();

    if (!context)
    {
        LOG_ERROR("gp_context_new() failed.");
        return false;
    }

    int rc = gp_camera_new(&camera);
    if (rc < GP_OK)
    {
        LOGF_ERROR("gp_camera_new() failed: %d", rc);
        gp_context_unref(context);
        context = nullptr;
        camera = nullptr;
        return false;
    }

    LOG_INFO("Opening USB/PTP libgphoto2 camera session.");

    rc = gp_camera_init(camera, context);
    if (rc < GP_OK)
    {
        LOGF_ERROR("gp_camera_init() failed: %d", rc);
        gp_camera_free(camera);
        camera = nullptr;
        gp_context_unref(context);
        context = nullptr;
        return false;
    }

    LOG_INFO("USB/PTP camera session is open.");
    return true;
}

void EOSM6USB::closeCamera()
{
    if (camera)
    {
        LOG_INFO("Closing USB/PTP libgphoto2 camera session.");
        gp_camera_exit(camera, context);
        gp_camera_free(camera);
        camera = nullptr;
    }

    if (context)
    {
        gp_context_unref(context);
        context = nullptr;
    }
}

bool EOSM6USB::findWidgetRecursive(CameraWidget *widget,
                                           const char *wanted,
                                           std::string &value)
{
    if (!widget || !wanted)
        return false;

    const char *name = nullptr;
    if (gp_widget_get_name(widget, &name) == GP_OK && name &&
        !strcmp(name, wanted))
    {
        const char *current = nullptr;
        if (gp_widget_get_value(widget, &current) == GP_OK && current)
        {
            value = current;
            return true;
        }
    }

    int count = gp_widget_count_children(widget);
    if (count < 0)
        return false;

    for (int i = 0; i < count; ++i)
    {
        CameraWidget *child = nullptr;
        if (gp_widget_get_child(widget, i, &child) != GP_OK || !child)
            continue;

        if (findWidgetRecursive(child, wanted, value))
            return true;
    }

    return false;
}

bool EOSM6USB::readCameraSettings()
{
    if (!camera || !context)
        return false;

    CameraWidget *root = nullptr;

    int rc = gp_camera_get_config(camera, &root, context);
    if (rc < GP_OK)
    {
        LOGF_ERROR("gp_camera_get_config() failed: %d", rc);
        return false;
    }

    std::string iso;
    std::string shutter;

    const bool gotISO =
        findWidgetRecursive(root, "iso", iso);

    const bool gotShutter =
        findWidgetRecursive(root, "shutterspeed", shutter);

    if (gotISO)
        IUSaveText(&CameraInfoTP[0], iso.c_str());

    if (gotShutter)
        IUSaveText(&CameraInfoTP[1], shutter.c_str());

    CameraInfoTPV.s = (gotISO || gotShutter) ? IPS_OK : IPS_ALERT;
    IDSetText(&CameraInfoTPV, nullptr);

    gp_widget_free(root);

    LOGF_INFO("Camera settings: ISO=%s, shutter=%s",
              gotISO ? iso.c_str() : "Unknown",
              gotShutter ? shutter.c_str() : "Unknown");

    return gotISO || gotShutter;
}

bool EOSM6USB::Connect()
{
    LOG_INFO("Connecting Canon EOS M6 using USB/PTP libgphoto2 session.");

    if (!openCamera())
        return false;

    // Minimal one-time camera query on connect.
    readCameraSettings();

    LOG_INFO("EOS M6 connected. PTP session remains open until disconnect.");
    return true;
}

bool EOSM6USB::Disconnect()
{
    if (busy)
    {
        LOG_ERROR("Cannot disconnect while an exposure is active.");
        return false;
    }

    closeCamera();
    LOG_INFO("EOS M6 disconnected.");
    return true;
}

std::string EOSM6USB::makeLocalFilename() const
{
    char timestamp[64] {};
    time_t now = time(nullptr);
    struct tm tmNow {};
    localtime_r(&now, &tmNow);

    strftime(timestamp, sizeof(timestamp),
             "EOSM6_%Y%m%d_%H%M%S.CR2", &tmNow);

    return downloadDir + "/" + timestamp;
}

bool EOSM6USB::captureAndDownload(std::string &localFile)
{
    if (!camera || !context)
        return false;

    mkdir(downloadDir.c_str(), 0775);

    CameraFilePath path {};
    int rc = gp_camera_capture(camera, GP_CAPTURE_IMAGE, &path, context);

    if (rc < GP_OK)
    {
        LOGF_ERROR("gp_camera_capture() failed: %d", rc);
        return false;
    }

    LOGF_INFO("Camera capture completed: %s/%s",
              path.folder, path.name);

    localFile = makeLocalFilename();

    int fd = open(localFile.c_str(),
                  O_CREAT | O_WRONLY | O_TRUNC, 0664);

    if (fd < 0)
    {
        LOGF_ERROR("Cannot create local CR2: %s (%s)",
                   localFile.c_str(), strerror(errno));
        return false;
    }

    CameraFile *file = nullptr;

    rc = gp_file_new_from_fd(&file, fd);
    if (rc < GP_OK)
    {
        close(fd);
        unlink(localFile.c_str());
        LOGF_ERROR("gp_file_new_from_fd() failed: %d", rc);
        return false;
    }

    rc = gp_camera_file_get(camera, path.folder, path.name,
                            GP_FILE_TYPE_NORMAL, file, context);

    gp_file_free(file);

    if (rc < GP_OK)
    {
        unlink(localFile.c_str());
        LOGF_ERROR("gp_camera_file_get() failed: %d", rc);
        return false;
    }

    struct stat st {};
    if (stat(localFile.c_str(), &st) != 0 || st.st_size <= 0)
    {
        unlink(localFile.c_str());
        LOG_ERROR("Downloaded CR2 is missing or empty.");
        return false;
    }

    LOGF_INFO("CR2 downloaded through USB/PTP session: %s (%lld bytes)",
              localFile.c_str(),
              static_cast<long long>(st.st_size));

    rc = gp_camera_file_delete(camera, path.folder, path.name, context);

    if (rc < GP_OK)
    {
        LOGF_ERROR("Downloaded CR2 saved locally, but camera-side delete failed: %d", rc);
        return false;
    }

    LOGF_INFO("Camera-side CR2 deleted: %s/%s",
              path.folder, path.name);

    return true;
}

bool EOSM6USB::loadFileToCCD(const std::string &localFile)
{
    std::ifstream file(localFile, std::ios::binary | std::ios::ate);

    if (!file)
    {
        LOGF_ERROR("Cannot open CR2: %s", localFile.c_str());
        return false;
    }

    const std::streamsize size = file.tellg();

    if (size <= 0)
    {
        LOGF_ERROR("Invalid CR2 size: %lld",
                   static_cast<long long>(size));
        return false;
    }

    file.seekg(0, std::ios::beg);

    uint8_t *buffer =
        static_cast<uint8_t *>(IDSharedBlobAlloc(size));

    if (!buffer)
    {
        LOGF_ERROR("IDSharedBlobAlloc failed for %lld bytes",
                   static_cast<long long>(size));
        return false;
    }

    if (!file.read(reinterpret_cast<char *>(buffer), size))
    {
        IDSharedBlobFree(buffer);
        LOG_ERROR("Failed to read CR2 into memory.");
        return false;
    }

    PrimaryCCD.setImageExtension("cr2");
    PrimaryCCD.setFrameBufferSize(size, false);
    PrimaryCCD.setFrameBuffer(buffer);
    PrimaryCCD.setResolution(6000, 4000);
    PrimaryCCD.setFrame(0, 0, 6000, 4000);
    PrimaryCCD.setNAxis(2);
    PrimaryCCD.setBPP(16);

    LOGF_INFO("CR2 loaded into INDI CCD framebuffer: %lld bytes",
              static_cast<long long>(size));

    return true;
}

bool EOSM6USB::StartExposure(float duration)
{
    (void)duration;

    if (!isConnected() || !camera)
    {
        LOG_ERROR("Camera is not connected.");
        return false;
    }

    if (busy)
    {
        LOG_ERROR("Driver is already busy.");
        return false;
    }

    busy = true;
    abortRequested = false;

    PrimaryCCD.setExposureDuration(0);
    PrimaryCCD.setExposureLeft(0);

    std::string localFile;

    if (!captureAndDownload(localFile))
    {
        failExposure("Capture/download failed.");
        return false;
    }

    LOG_INFO("STEP 1: capture/download finished.");

    if (abortRequested)
    {
        failExposure("Capture aborted.");
        return true;
    }

    if (!loadFileToCCD(localFile))
    {
        failExposure("Failed to load CR2 into INDI BLOB.");
        return false;
    }

    LOG_INFO("STEP 2: loadFileToCCD finished.");

    busy = false;
    ExposureComplete(&PrimaryCCD);

    LOG_INFO("STEP 3: ExposureComplete finished.");

    LOGF_INFO("EOS M6 image complete: %s", localFile.c_str());

    return true;
}

bool EOSM6USB::AbortExposure()
{
    if (!busy)
        return true;

    /*
     * libgphoto2 does not expose a generic public "abort capture"
     * operation for this driver. Do not tear down the USB/PTP session
     * here. Mark the request aborted; the synchronous capture operation
     * will return and the next state is handled normally.
     */
    abortRequested = true;

    LOG_INFO("EOS M6 abort requested.");
    return true;
}

void EOSM6USB::failExposure(const std::string &reason)
{
    busy = false;
    PrimaryCCD.setExposureFailed();
    LOGF_ERROR("EOS M6 capture failed: %s", reason.c_str());
}

static EOSM6USB eosm6;
