/*
 * This file Copyright (C) 2019 t-flo
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id$
 */
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <libtransmission/transmission.h>
#include <libtransmission/error.h>
#include <libtransmission/file.h>
#include <libtransmission/tr-getopt.h>
#include <libtransmission/log.h>
#include <libtransmission/utils.h>
#include <libtransmission/variant.h>
#include <libtransmission/version.h>

#include <switch.h>

#define MY_NAME "nxTransmission"

#define NXLINK 0
#define LOGTOFILE 0 // 1 if log to 
#define LOGFILE "sdmc:/switch/nxTransmission/log.txt"

#define SPLIT_FILE_SIZE 0xFFFF0000 // 4,294,901,760 bytes
#define CHUNKSIZE 0x8000 // 32,768 bytes

#define SOCK_BUFFERSIZE 0x10000 
u32 __nx_fs_num_sessions = 8;

#define DEFAULT_RPC_USERNAME "switch"
#define DEFAULT_RPC_PASSWORD "switch"
#define DEFAULT_RPC_PORT 9091

#define CONSOLE_WIDTH 80
#define CONSOLE_HEIGHT 45

#define ESC(x) "\x1b[" #x
#define RESET ESC(0m)
#define GREEN ESC(32;1m)
#define YELLOW ESC(33;1m)
#define RED ESC(31;1m)
#define BLUE ESC(34;1m)

#define MEM_K 1024
#define MEM_K_STR "KiB"
#define MEM_M_STR "MiB"
#define MEM_G_STR "GiB"
#define MEM_T_STR "TiB"

#define DISK_K 1000
#define DISK_B_STR  "B"
#define DISK_K_STR "kB"
#define DISK_M_STR "MB"
#define DISK_G_STR "GB"
#define DISK_T_STR "TB"

#define SPEED_K 1000
#define SPEED_B_STR  "B/s"
#define SPEED_K_STR "kB/s"
#define SPEED_M_STR "MB/s"
#define SPEED_G_STR "GB/s"
#define SPEED_T_STR "TB/s"

#if LOGTOFILE
static tr_sys_file_t logfile = TR_BAD_SYS_FILE;
#else
static const tr_sys_file_t logfile = TR_STD_SYS_FILE_OUT;
#endif
static tr_session * mySession = NULL;
static tr_variant settings;
static const char * configDir = "sdmc:/switch/nxTransmission";

static char listen_addr[20];
static PrintConsole status_console;
static PrintConsole main_console;

static void
init_console (void)
{
    consoleInit(&status_console);
    consoleSetWindow(&status_console, 0, 0, CONSOLE_WIDTH, 6);

    consoleInit(&main_console);
    consoleSetWindow(&main_console, 0, 7, CONSOLE_WIDTH, CONSOLE_HEIGHT - 7);

    consoleSelect(&main_console);
}

__attribute__ ((format (gnu_printf, 1, 2)))
static void
console_update_status (const char *fmt, ...)
{
#if !NXLINK
    va_list args;

    consoleSelect(&status_console);
    consoleClear();

    printf(YELLOW "nxTransmission %s   --" RED "   Press B to exit.\n", VERSION_STRING);

    printf(GREEN "RPC: http://%s:%i  Default username/password: %s/%s\n\n",
        listen_addr,
        DEFAULT_RPC_PORT,
        DEFAULT_RPC_USERNAME,
        DEFAULT_RPC_PASSWORD);

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    consoleSelect(&main_console);
#endif
}

static void
press_b_again_to_exit (void)
{
    console_update_status("Press B again to exit.\n");
    consoleUpdate(NULL);

    while(appletMainLoop())
    {       
        hidScanInput();
        if (hidKeysDown(CONTROLLER_P1_AUTO) & (KEY_B | KEY_PLUS))
            break;
    }
}

#if LOGTOFILE
static bool
reopen_log_file (const char *filename)
{
    tr_error * error = NULL;
    const tr_sys_file_t old_log_file = logfile;
    const tr_sys_file_t new_log_file = tr_sys_file_open (filename,
                                                         TR_SYS_FILE_WRITE | TR_SYS_FILE_CREATE | TR_SYS_FILE_APPEND,
                                                         0666, &error);

    if (new_log_file == TR_BAD_SYS_FILE)
    {
        fprintf (stderr, "Couldn't (re)open log file \"%s\": %s\n", filename, error->message);
        tr_error_free (error);
        return false;
    }

    logfile = new_log_file;

    if (old_log_file != TR_BAD_SYS_FILE)
        tr_sys_file_close (old_log_file, NULL);

    return true;
}
#endif

static char *
nxbasename (const char *filename)
{
  char * p = strrchr (filename, '/');
  return p ? p + 1 : (char *) filename;
}

static void
printMessage (tr_sys_file_t logfile, int level, const char * name, const char * message, const char * file, int line)
{
    if (logfile != TR_BAD_SYS_FILE)
    {
        char timestr[64];
        tr_logGetTimeStr (timestr, sizeof (timestr));
        if (name)
            tr_sys_file_write_fmt (logfile, "[%s] %s %s (%s:%d)" TR_NATIVE_EOL_STR,
                                   NULL, timestr, name, message, nxbasename(file), line);
        else
            tr_sys_file_write_fmt (logfile, "[%s] %s (%s:%d)" TR_NATIVE_EOL_STR,
                                   NULL, timestr, message, nxbasename(file), line);
    }
    (void) level;
}

static void
pumpLogMessages (tr_sys_file_t logfile)
{
    const tr_log_message * l;
    tr_log_message * list = tr_logGetQueue ();

    for (l=list; l!=NULL; l=l->next)
        printMessage (logfile, l->level, l->name, l->message, nxbasename(l->file), l->line);

    if (logfile != TR_BAD_SYS_FILE)
        tr_sys_file_flush (logfile, NULL);

    tr_logFreeQueue (list);
}

static void
reportStatus (void)
{
    const double up = tr_sessionGetRawSpeed_KBps (mySession, TR_UP);
    const double dn = tr_sessionGetRawSpeed_KBps (mySession, TR_DOWN);

    if (up>0 || dn>0)
        console_update_status ("Uploading %.2f KBps, Downloading %.2f KBps.\n", up, dn);
    else
        console_update_status ("Idle.\n");
}

static void
periodic_update (void)
{
    pumpLogMessages (logfile);
    reportStatus ();
}

typedef void (*splitFileActionFunc)(const char * path);

static void
handleSplitFiles(tr_torrent * tor, splitFileActionFunc action)
{
    const tr_info * info = tr_torrentInfo(tor);

    uint32_t i = 0;
    while(i < info->fileCount)
    {
        tr_file * file = &info->files[i];
        if(!file->splitFile)
        {
            i++;
            continue;
        }

        tr_file * firstSplitFile = file;
        char * fileName = tr_torrentFindFile(tor, i);
        if(fileName == NULL)
        {
            i++;
            continue;
        }

        char * dirName = tr_sys_path_dirname(fileName, NULL);

        action(dirName);

        tr_free(dirName);
        tr_free(fileName);

        i++;

        while(i < info->fileCount)
        {
            file = &info->files[i];
            if(strncmp(firstSplitFile->name, file->name, strlen(firstSplitFile->name)-3) != 0)
                break;
            i++;
        }
    }
}

static void removeSplitFile(const char * path)
{
    tr_sys_path_remove(path, NULL);
}

static void applyConcatenationFileAttributeToSplitFile(const char * path)
{
    fsdevSetConcatenationFileAttribute(path);
}

static void
on_torrent_completed (tr_torrent * tor,
                      tr_completeness completeness,
                      bool was_running UNUSED,
                      void * data UNUSED)
{
    if(completeness != TR_SEED)
        return;

    handleSplitFiles(tor, &applyConcatenationFileAttributeToSplitFile);
}

static tr_rpc_callback_status
on_rpc_callback (tr_session            * session UNUSED,
                 tr_rpc_callback_type    type,
                 struct tr_torrent     * tor,
                 void                  * user_data UNUSED)
{
    if (type == TR_RPC_TORRENT_ADDED)
        tr_torrentSetCompletenessCallback(tor, &on_torrent_completed, NULL);
    else if(type == TR_RPC_TORRENT_TRASHING)
        handleSplitFiles(tor, &removeSplitFile);
    return TR_RPC_OK;
}

static void
stop_session (void)
{
    console_update_status ("Exiting cleanly... (With DHT enabled this might take a couple of seconds)\n");
    consoleUpdate (NULL);

    tr_sessionSaveSettings (mySession, configDir, &settings);
    tr_sessionClose (mySession);
    mySession = NULL;
    pumpLogMessages (logfile);
}

static bool
start_session (void)
{
    bool boolVal;
    tr_session * session = NULL;

    console_update_status ("Starting...\n");

    /* should go before libevent calls */
    tr_net_init ();

    /* start the session */
    tr_formatter_mem_init (MEM_K, MEM_K_STR, MEM_M_STR, MEM_G_STR, MEM_T_STR);
    tr_formatter_size_init (DISK_K, DISK_K_STR, DISK_M_STR, DISK_G_STR, DISK_T_STR);
    tr_formatter_speed_init (SPEED_K, SPEED_K_STR, SPEED_M_STR, SPEED_G_STR, SPEED_T_STR);
    session = tr_sessionInit (configDir, true, &settings);
    tr_sessionSetRPCCallback (session, on_rpc_callback, NULL);
    tr_logAddNamedInfo (NULL, "Using settings from \"%s\"", configDir);
    tr_sessionSaveSettings (session, configDir, &settings);

    if (tr_variantDictFindBool (&settings, TR_KEY_rpc_authentication_required, &boolVal) && boolVal)
        tr_logAddNamedInfo (MY_NAME, "requiring authentication");

    mySession = session;

    /* load the torrents */
    tr_ctor * ctor = tr_ctorNew (mySession);
    
    int count; 
    tr_torrent ** torrents;

    torrents = tr_sessionLoadTorrents (mySession, ctor, &count);
    for(int i = 0; i < count; i++)
        tr_torrentSetCompletenessCallback(torrents[i], &on_torrent_completed, NULL);

    tr_free (torrents);
    tr_ctorFree (ctor);

    console_update_status("Ready.\n");
    return true;
}

static void
main_loop (void)
{
    while(appletMainLoop ())
    {
        hidScanInput ();
        u64 key_down = hidKeysDown (CONTROLLER_P1_AUTO);

        if (key_down & KEY_B)
            break;

        tr_wait_msec (100);
        periodic_update ();
        consoleUpdate (NULL);
        tr_switch_join_finished_threads ();
    }
}

static int
tr_main_impl (bool isExfat)
{
    int ret = 1;

    /* load settings from defaults + config file */
    tr_variantInitDict (&settings, 0);
    tr_variantDictAddBool (&settings, TR_KEY_rpc_enabled, true);

#if LOGTOFILE
    reopen_log_file (LOGFILE);
#endif

    if (!tr_sessionLoadSettings (&settings, configDir, MY_NAME))
    {
        printMessage (logfile, TR_LOG_ERROR, MY_NAME, "Error loading config file -- exiting.", nxbasename(__FILE__), __LINE__);
        goto cleanup;
    }
    
    /* if nx-split-files is not defined in settings, set it if exFat is not available */
    bool boolVal = false;
    if(!tr_variantDictFindBool (&settings, TR_KEY_nxSplitFiles, &boolVal) && !isExfat)
    {
        tr_variantDictAddBool (&settings, TR_KEY_nxSplitFiles, true);
        tr_variantDictAddInt (&settings, TR_KEY_nxSplitSize, SPLIT_FILE_SIZE);
    }

    /* if the split size cannot be dividev by chunk size(32 768), the system won't handle correctly the split files. */
    int64_t int64Val;
    if(tr_variantDictFindInt (&settings, TR_KEY_nxSplitSize, &int64Val))
    {
        if(int64Val % CHUNKSIZE)
        {
            char buf[256];
            tr_snprintf (buf, sizeof (buf), "nx-split-size = %ld cannot be divided by chunk size, reset nx-split-size to %u\n", int64Val, SPLIT_FILE_SIZE);
            printMessage (logfile, TR_LOG_ERROR, MY_NAME, buf, nxbasename(__FILE__), __LINE__);
            tr_variantDictAddInt (&settings, TR_KEY_nxSplitSize, SPLIT_FILE_SIZE);
        }
    }

    if (!start_session ())
    {
        printMessage (logfile, TR_LOG_ERROR, MY_NAME, "Failed to start session.\n", nxbasename(__FILE__), __LINE__);
        goto cleanup;
    }

    main_loop ();
    stop_session ();
    ret = 0;

cleanup:
    tr_variantFree (&settings);

    return ret;
}

int main(void)
{
    int ret = -1;

    init_console();

    static const SocketInitConfig socketInitConfig = {
        .bsdsockets_version = 1,
        .bsd_service_type = BsdServiceType_System,

        .tcp_tx_buf_size = SOCK_BUFFERSIZE,
        .tcp_rx_buf_size = SOCK_BUFFERSIZE,
        .tcp_tx_buf_max_size = SOCK_BUFFERSIZE,
        .tcp_rx_buf_max_size = SOCK_BUFFERSIZE,

        .udp_tx_buf_size = SOCK_BUFFERSIZE,
        .udp_rx_buf_size = SOCK_BUFFERSIZE,

        .sb_efficiency = 32,
        .num_bsd_sessions = 8
        };

    int rc;
    if (R_FAILED(rc = socketInitialize(&socketInitConfig)))
    {
        printf("socketInitializeDefault() failed: 0x%x.\n\n", rc);
        goto cleanup;
    }
    
    bool isExfat = false;
    if (R_FAILED(rc = fsIsExFatSupported(&isExfat)))
    {
        printf("fsIsExFatSupported() failed: 0x%x.\n\n", rc);
        goto cleanup;
    }

#if NXLINK
    nxlinkStdio();
#else
    consoleDebugInit(debugDevice_NULL);
#endif

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = gethostid();
    strcpy(listen_addr, inet_ntoa(serv_addr.sin_addr));
    
    tr_switch_init();

    ret = tr_main_impl(isExfat);

    tr_switch_exit();
    
cleanup:
    press_b_again_to_exit();

    socketExit();
    consoleExit(NULL);

    return ret;
}
