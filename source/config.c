#include "config.h"

static bootNtrConfig_t  g_bnConfig = { 0 };
static ntrConfig_t      g_ntrConfig = { 0 };
bootNtrConfig_t         *bnConfig;
ntrConfig_t             *ntrConfig;

static const char   *configPath = "/3ds/BootNTRSelector/config";
static const char   *configDir = "/3ds/BootNTRSelector/";

bool    checkPath(void)
{
    char    *tmp;

    if (strncmp(bnConfig->config->binariesPath, "sdmc:/", 6)) goto error;
    if (strncmp(bnConfig->config->pluginPath, "sdmc:/", 6)) goto error;
    tmp = bnConfig->config->binariesPath;
    while (*(tmp + 1)) tmp++;
    if (*tmp != '/') *(tmp + 1) = '/';
    tmp = bnConfig->config->pluginPath;
    while (*(tmp + 1)) tmp++;
    if (*tmp != '/') *(tmp + 1) = '/';
    return (true);
error:
    if (*bnConfig->config->binariesPath == '/')
        strInsert(bnConfig->config->binariesPath, "sdmc:", 0);
    else
        strInsert(bnConfig->config->binariesPath, "sdmc:/", 0);
    if (*bnConfig->config->pluginPath == '/')
        strInsert(bnConfig->config->pluginPath, "sdmc:", 0);
    else
        strInsert(bnConfig->config->pluginPath, "sdmc:/", 0);
    return (checkPath());
}

bool    loadConfigFromFile(config_t *config)
{
    FILE        *file = NULL;

    if (!config) goto error;
    //if (!fileExists(configPath)) goto error;
    file = fopen(configPath, "rb");
    if (!file) goto error;
    fread(config, sizeof(config_t), 1, file);
    fclose(file);

    // Check version of the config file
    if (config->version != CURRENT_CONFIG_VERSION)
    {
        memset(config, 0, sizeof(config_t));
        config->version = CURRENT_CONFIG_VERSION;
        g_bnConfig.checkForUpdate = true;
        goto error;
    }

    return (true);
error:
    return (false);
}

bool    saveConfig(void)
{
    FILE        *file = 0;
    config_t    *config = g_bnConfig.config;

    if (!config)
        goto error;
    if (!fileExists(configDir))
        createDir(configDir);
    if (!fileExists(configDir)) goto error;
    file = fopen(configPath, "wb");
    if (!file) goto error;
    fwrite(config, sizeof(config_t), 1, file);
    fflush(file);
    fclose(file);
    return (true);
error:
    return (false);
}

void    resetConfig(void)
{
    //char        path[0x100];
    config_t    *config = NULL;
    //bool        binPath = false;
    //u32         keys;
    //u32         size;

    if (!fileExists(configPath)) goto exit;
    config = (config_t *)calloc(1, sizeof(config_t));
    if (!config)
        goto exit;
    //binPath = loadConfigFromFile(config);
    remove(configPath);
    rmdir(configDir);
   /* if (!binPath) goto exit;
    size = strlen(config->binariesPath);
    if (!size) goto exit;
    memset(path, 0, 0x100);
    strJoin(path, config->binariesPath + 5, "ntr_3_2.bin");
    remove(path);
    memset(path, 0, 0x100);
    strJoin(path, config->binariesPath + 5, "ntr_3_3.bin");
    remove(path);
    memset(path, 0, 0x100);
    strJoin(path, config->binariesPath + 5, "ntr_3_6.bin");
    remove(path); */

exit:
    if (config)
        free(config);
    return;
}

void    configInit(void)
{
    config_t    *config;
    Handle      fsuHandle = *fsGetSessionHandle();
    bool        isNew3DS = false;;


   // srvGetServiceHandle(&fsuHandle, "fs:USER");
   //  FSUSER_Initialize(fsuHandle);

    APT_CheckNew3DS(&isNew3DS);

    memset(&g_ntrConfig, 0, sizeof(g_ntrConfig));
    memset(&g_bnConfig, 0, sizeof(g_bnConfig));

    ntrConfig = &g_ntrConfig;
    bnConfig = &g_bnConfig;
    ntrConfig->fsUserHandle = fsuHandle;
    g_bnConfig.isMode3 = EXTENDEDMODE;
    g_bnConfig.isDebug = DEBUGMODE;
    g_bnConfig.isNew3DS = isNew3DS;

    config = (config_t *)calloc(1, sizeof(config_t));
    if (!config) goto error;
    g_bnConfig.config = config;
    if (!loadConfigFromFile(config))
    {
        saveConfig();
        firstLaunch();
        if (!saveConfig())
            newAppTop(DEFAULT_COLOR, 0, "A problem occured while saving the settings.");
        if (g_bnConfig.isMode3)
            g_bnConfig.versionToLaunch = V36;
    }
    else
    {
        time_t current = time(NULL);
        time_t last;

        if (envIsHomebrew())
            last = config->lastUpdateTime3dsx;
        else
            last = g_bnConfig.isMode3 ? config->lastUpdateTime3 : config->lastUpdateTime;

        if (current - last >= SECONDS_IN_WEEK)
            g_bnConfig.checkForUpdate = true;
        else
            g_bnConfig.checkForUpdate = false;
    }

    if (g_bnConfig.isMode3)
    {
        g_bnConfig.versionToLaunch = V36;
    }
    else
    {
        if (config->flags & LV32) g_bnConfig.versionToLaunch = V32;
        else if (config->flags & LV33) g_bnConfig.versionToLaunch = V33;
        else if (config->flags & LV36) g_bnConfig.versionToLaunch = V36;
    }
error:
    return;
}

void    configExit(void)
{
    version_t   version;
    config_t    *config;
    u32         flags;

    version = g_bnConfig.versionToLaunch;
    config = g_bnConfig.config;
    if (!g_bnConfig.isMode3)
    {
        if (version == V32) flags = LV32;
        else if (version == V33) flags = LV33;
        else if (version == V36) flags = LV36;
        else flags = 0;
        config->flags = flags;
    }

    saveConfig();
    free(config);
    g_bnConfig.config = NULL;
}
