# 升级模块

### 使用
```
DjiCore_SetAlias("MP20"); // 设置设备名称
if (CziUpgrade_StartService(firmwareVersion) != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //启动设计模块并设置firmwareVersion版本号
    USER_LOG_ERROR("psdk upgrade init error");
}
```

### firmwareVersion信息
```
static T_DjiFirmwareVersion firmwareVersion = {
    .majorVersion = 1,
    .minorVersion = 0,
    .modifyVersion = 0,
    .debugVersion = 20,
};
```
其中T_DjiFirmwareVersion结构体为大疆设置的版本号设置。
