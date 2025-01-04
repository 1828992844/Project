/**
 ********************************************************************
 * @file    main.c
 * @brief
 *
 * @copyright (c) 2021 DJI. All rights reserved.
 *
 * All information contained herein is, and remains, the property of DJI.
 * The intellectual and technical concepts contained herein are proprietary
 * to DJI and may be covered by U.S. and foreign patents, patents in process,
 * and protected by trade secret or copyright law.  Dissemination of this
 * information, including but not limited to data and other proprietary
 * material(s) incorporated within the information, in any form, is strictly
 * prohibited without the express written consent of DJI.
 *
 * If you receive this source code without DJI’s authorization, you may not
 * further disseminate the information, and you must immediately remove the
 * source code and notify DJI of its removal. DJI reserves the right to pursue
 * legal actions against you for any loss(es) or damage(s) caused by your
 * failure to do so.
 *
 *********************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <unistd.h>
#include <signal.h>
#include "czi_multi_task.h"
#include "czi_logClient.h"
#include "czi_video.h"


/* Private constants ---------------------------------------------------------*/

/* Private types -------------------------------------------------------------*/

/* Private values -------------------------------------------------------------*/


/* Private functions declaration ---------------------------------------------*/


/* Exported functions definition ---------------------------------------------*/
T_CziLogUserFormat user = 
{
    .logServerLocationPath = CZI_USER_LOG_LOCATION,
    .logFormat = CZI_USER_LOG_FORMAT,
    .logTag = CZI_USER_LOG_TAG,
};

int main(int argc, char **argv)
{
    // USER_UTIL_UNUSED(argc);
    // USER_UTIL_UNUSED(argv);
    Czilog_ClientInit(&user);
    CziLog_Error("Czilog_ClientInit init sucess\n");
    signal(SIGINT,  CziMultiTask_NormalExitHandler);
    signal(SIGTERM, CziMultiTask_NormalExitHandler);
    CziMultiTask_ApplicationStart(NULL);


    

    // 以下是启动视频流获取的代码
    // T_DjiReturnCode returnCode;
    // returnCode = CziVideo_RunSample();
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     printf("Failed to run video sample\n");
    //     CziVideo_Deinit();
    //     return -1;
    // }

    // returnCode = CziVideo_Deinit();
    // if (returnCode != DJI_ERROR_SYSTEM_MODULE_CODE_SUCCESS) {
    //     printf("Failed to deinitialize video module\n");
    //     return -1;
    // }

    // printf("Video stream sample completed successfully\n");
    while (1) {
        sleep(1);
    }
}





#pragma GCC diagnostic pop

/****************** (C) COPYRIGHT DJI Innovations *****END OF FILE****/
