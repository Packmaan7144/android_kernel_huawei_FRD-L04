

/*****************************************************************************
  1 头文件包含
*****************************************************************************/


#include "NasFsm.h"
#include "NasMmcTimerMgmt.h"
#include "MsccMmcInterface.h"

#include "NasMmcPreProcAct.h"
#include "NasMmcPreProcTbl.h"
#include "UsimPsInterface.h"
#include "MmcGmmInterface.h"
#include "MmcMmInterface.h"
#if (FEATURE_ON == FEATURE_LTE)
#include "MmcLmmInterface.h"
#endif
#include "NasOmInterface.h"
#include "NasMmcCtx.h"
/* 删除ExtAppMmcInterface.h*/
#include "NasMmcSndOm.h"
#include "NasMmcSndInternalMsg.h"
#include "NasMmcFsmMainTbl.h"
#include  "siappstk.h"

#if (FEATURE_ON == FEATURE_PTM)
#include "NasErrorLogGu.h"
#endif

#include "PsRrmInterface.h"
#include "NasMtaInterface.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define    THIS_FILE_ID        PS_FILE_ID_NAS_MMC_PREPROCTBL_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/* 预处理状态机 */
NAS_FSM_DESC_STRU                       g_stNasMmcPreFsmDesc;


/*新增状态动作处理表 */

/* 不进状态机处理的消息 动作表 */
NAS_ACT_STRU        g_astNasMmcPreProcessActTbl[]   =
{
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_ATTACH_REQ,
                      NAS_MMC_RcvMsccAttachReq_PreProc),

    NAS_ACT_TBL_ITEM( MAPS_PIH_PID,
                      USIMM_STKREFRESH_IND,
                      NAS_MMC_RcvUsimRefreshFileInd_PreProc),

   NAS_ACT_TBL_ITEM( MAPS_PIH_PID,
                     USIMM_STKREFRESH_TYPE_IND,
                     NAS_MMC_RcvUsimRefreshTypeInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_UPDATE_UPLMN_NTF,
                      NAS_MMC_RcvMsccUpdateUplmnNtf_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_DETACH_REQ,
                      NAS_MMC_RcvMsccDetachReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_MODE_CHANGE_REQ,
                      NAS_MMC_RcvMsccModeChange_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PLMN_SPECIAL_REQ,
                      NAS_MMC_RcvMsccUserSpecPlmnSearch_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PLMN_USER_RESEL_REQ,
                      NAS_MMC_RcvUserReselReq_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_AVAILABLE_TIMER,
                      NAS_MMC_RcvTiAvailTimerExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                     TI_NAS_MMC_PLMN_SEARCH_PHASE_ONE_TOTAL_TIMER,
                     NAS_MMC_RcvTiPlmnSearchPhaseOneTotalTimerExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_TRYING_USER_PLMN_LIST,
                      NAS_MMC_RcvTiPeriodTryingUserPlmnListExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_TRYING_INTER_PLMN_LIST,
                      NAS_MMC_RcvTiPeriodTryingInterPlmnListExpired_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PLMN_LIST_REQ,
                      NAS_MMC_RcvTafPlmnListReq_PreProc),

#if (FEATURE_ON == FEATURE_CSG)
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CSG_LIST_SEARCH_REQ,
                      NAS_MMC_RcvMsccCsgListSearchReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CSG_LIST_ABORT_REQ,
                      NAS_MMC_RcvCsgListAbortReq_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                     TI_NAS_MMC_PERIOD_TRYING_USER_CSG_LIST_SEARCH,
                     NAS_MMC_RcvTiPeriodTryingUserCsgListExpired_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CSG_SPEC_SEARCH_REQ,
                      NAS_MMC_RcvMsccUserCsgSpecSearchReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CSG_SPEC_SEARCH_ABORT_REQ,
                      NAS_MMC_RcvMsccUserCsgSpecSearchAbortReq_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_DELETE_FORBIDDEN_CSG_ID_TIMER,
                      NAS_MMC_RcvPeriodDeleteForbiddenCsgIdTimerExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_AUTONOMOUS_CSG_ID_SEARCH_TIMER,
                      NAS_MMC_RcvTiAutonomousCsgIdSearchExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_CSG_ID_SEARCH_TIMER,
                      NAS_MMC_RcvTiPeriodCsgIdSearchExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_TRYING_CSG_ID_SEARCH,
                      NAS_MMC_RcvTiPeriodTryingCsgIdSearchExpired_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_USIM,
                      USIMM_QUERYFILE_CNF,
                      NAS_MMC_RcvUsimQueryFileCnf_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_USIM,
                      USIMM_READFILE_CNF,
                      NAS_MMC_RcvUsimReadFileCnf_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_WAIT_READ_SIM_FILES,
                      NAS_MMC_RcvTiReadSimFilesExpired_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_CSG_ID_HOME_NODEB_NAME_IND,
                      NAS_MMC_RcvCsgIdHomeNodeBNameInd_PreProc),
#endif
    NAS_ACT_TBL_ITEM( WUEPS_PID_MMC,
                      MMCMMC_INTER_PLMN_LIST_REQ,
                      NAS_MMC_RcvMmcInterPlmnListReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PLMN_LIST_ABORT_REQ,
                      NAS_MMC_RcvTafPlmnListAbortReq_PlmnList_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_SPEC_PLMN_SEARCH_ABORT_REQ,
                      NAS_MMC_RcvTafSpecPlmnSearchAbortReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_STOP_GET_GEO_REQ,
                      NAS_MMC_RcvMsccStopGetGeoReq_PreProc),


    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_DPLMN_SET_REQ,
                      NAS_MMC_RcvMsccDplmnSetReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_EOPLMN_SET_REQ,
                      NAS_MMC_RcvMsccEOPlmnSetReq_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MMC,
                      MMCMMC_INTER_NVIM_OPLMN_REFRESH_IND,
                      NAS_MMC_RcvMmcInterNvimOPlmnRefreshInd_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_HPLMN_TIMER,
                      NAS_MMC_RcvTiHplmnTimerExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_TRYING_HIGH_PRIO_PLMN_SEARCH,
                      NAS_MMC_RcvTiTryingHighPrioPlmnSearchExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_HIGH_PRIO_RAT_HPLMN_TIMER,
                      NAS_MMC_RcvTiHighPrioRatHplmnSrchTimerExpired_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MMC,
                      MMCMMC_INTER_BG_SEARCH_REQ,
                      NAS_MMC_RcvMmcInterBgSearchReq_PreProc),

    NAS_ACT_TBL_ITEM( MAPS_STK_PID,
                      STK_NAS_STEERING_OF_ROAMING_IND,
                      NAS_MMC_RcvStkSteerRoamingInd_PreProc),


    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_LOCAL_DETACH_IND,
                      NAS_MMC_RcvGmmLocalDetachInd_PreProc),

#if   (FEATURE_ON == FEATURE_LTE)

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_TIN_TYPE_IND,
                      NAS_MMC_RcvGmmTinInd_PreProc ),
#endif

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      GMMMMC_NETWORK_CAPABILITY_INFO_IND,
                      NAS_MMC_RcvGmmNetworkCapabilityInfoInd_PreProc ),


    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_INFO_IND,
                      NAS_MMC_RcvGmmInfo_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_PDP_STATUS_IND,
                      NAS_MMC_RcvGmmPdpStatusInd_PreProc ),




    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_INFO_IND,
                      NAS_MMC_RcvMmInfo_PreProc ),



    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_ATTACH_CNF,
                      NAS_MMC_RcvMmAttachCnf_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_ATTACH_CNF,
                      NAS_MMC_RcvGmmAttachCnf_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_DETACH_CNF,
                      NAS_MMC_RcvMmDetachCnf_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_DETACH_CNF,
                      NAS_MMC_RcvGmmDetachCnf_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMMMC_CS_REG_RESULT_IND,
                      NAS_MMC_RcvMmCsRegResultInd_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      GMMMMC_PS_REG_RESULT_IND,
                      NAS_MMC_RcvGmmPsRegResultInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      GRRMM_SCELL_MEAS_IND,
                      NAS_MMC_RcvGasScellRxInd_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_AT_MSG_IND,
                      NAS_MMC_RcvWasAtMsgInd_PreProc ),
    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_W_AC_INFO_CHANGE_IND,
                      NAS_MMC_RcvWasAcInfoChange_PreProc ),
    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_AT_MSG_CNF,
                      NAS_MMC_RcvWasAtMsgCnf_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_PLMN_QUERY_REQ,
                      NAS_MMC_RcvRrMmPlmnQryReqPreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_NOT_CAMP_ON_IND,
                      NAS_MMC_RcvRrMmNotCampOn_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_NOT_CAMP_ON_IND,
                      NAS_MMC_RcvRrMmNotCampOn_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_EPLMN_QUERY_REQ,
                      NAS_MMC_RcvRrMmEquplmnQuery_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_EPLMN_QUERY_REQ,
                      NAS_MMC_RcvRrMmEquplmnQuery_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_LIMIT_SERVICE_CAMP_IND,
                      NAS_MMC_RcvRrMmLimitServiceCamp_PreProc ),


    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_SUSPEND_CNF,
                      NAS_MMC_RcvRrMmSuspendCnf_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_SUSPEND_CNF,
                      NAS_MMC_RcvRrMmSuspendCnf_PreProc ),


/* NAS_MMC_RcvRrMmGetTransactionReq_PreProc移到MM处理 */


    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_REL_IND,
                      NAS_MMC_RcvRrMmRelInd_PreProc ),



    /* MM发送的链接请求 */
    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_RR_CONN_INFO_IND,
                      NAS_MMC_RcvMmRrConnInfoInd_PreProc),


#if   (FEATURE_ON == FEATURE_LTE)

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_TIN_TYPE_IND,
                      NAS_MMC_RcvLmmTinInd_PreProc ),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_DETACH_CNF,
                      NAS_MMC_RcvLmmDetachCnf_PreProc ),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_ATTACH_CNF,
                      NAS_MMC_RcvLmmAttachCnf_PreProc ),

    /* L模上报出错的处理 */
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_ERR_IND,
                      NAS_MMC_RcvLmmErrInd_PreProc),

    /* 删除LTE上报服务状态和注册状态的接口 */
    /* NAS_MMC_RcvLmmRegStatusChangeInd_PreProc()/NAS_MMC_RcvLmmServiceStatusInd_PreProc处理函数 */


    /* LMM的本地detach的预处理 */
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_DETACH_IND,
                      NAS_MMC_RcvLmmMmcDetachInd_PreProc),

    /* LMM的挂起回复的预处理 */
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_SUSPEND_CNF,
                      NAS_MMC_RcvLmmSuspendCnf_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_STATUS_IND,
                      NAS_MMC_RcvLmmMmcStatusInd_PreProc),


    /* 收到LMM T3412或T3423定时器运行状态消息的预处理 */
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_TIMER_STATE_NOTIFY,
                      NAS_MMC_RcvLmmTimerStateNotify_PreProc),

    /* 增加LMM下转入非驻留态的消息处理 */
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_NOT_CAMP_ON_IND,
                      NAS_MMC_RcvLmmMmcNotCampOnInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_SERVICE_RESULT_IND,
                      NAS_MMC_RcvLmmServiceRsltInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_EMM_INFO_IND,
                      NAS_MMC_RcvLmmEmmInfoInd_PreProc),



    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_EMC_PDP_STATUS_NOTIFY,
                      NAS_MMC_RcvLmmEmcPdpStatusNotify_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_AREA_LOST_IND,
                      NAS_MMC_RcvLmmAreaLostInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_SIM_AUTH_FAIL_IND,
                      NAS_MMC_RcvLmmSimAuthFailInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_CELL_SIGN_REPORT_IND,
                      NAS_MMC_RcvLmmCellSignReportInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_ATTACH_IND,
                      NAS_MMC_RcvLmmAttachInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_TAU_RESULT_IND,
                      NAS_MMC_RcvLmmTauResultInd_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_SEARCHED_PLMN_INFO_IND,
                      NAS_MMC_RcvLmmSearchedPlmnInfoInd_PreProc),
#endif

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_SYS_CFG_SET_REQ,
                      NAS_MMC_RcvTafSysCfgReq_PreProc),


    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_ACQ_REQ,
                      NAS_MMC_RcvTafAcqReq_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_REG_REQ,
                      NAS_MMC_RcvTafRegReq_PreProc),

    NAS_ACT_TBL_ITEM( MSP_PID_DIAG_APP_AGENT,
                      OM_NAS_OTA_REQ,
                      NAS_MMC_RcvOmOtaReq_PreProc),

    NAS_ACT_TBL_ITEM( MSP_PID_DIAG_APP_AGENT,
                      ID_NAS_OM_MM_INQUIRE,
                      NAS_MMC_RcvOmInquireReq_PreProc),

    NAS_ACT_TBL_ITEM( MSP_PID_DIAG_APP_AGENT,
                      ID_NAS_OM_CONFIG_TIMER_REPORT_REQ,
                      NAS_MMC_RcvOmConfigTimerReportReq_PreProc),

#ifdef __PS_WIN32_RECUR__
    NAS_ACT_TBL_ITEM( WUEPS_PID_MMC,
                      MMCOM_OUTSIDE_RUNNING_CONTEXT_FOR_PC_REPLAY,
                      NAS_MMC_RestoreContextData_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MMC,
                      MMCOM_FIXED_PART_CONTEXT,
                      NAS_MMC_RestoreFixedContextData_PreProc),

#endif

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      GMMMMC_PS_REG_RESULT_IND,
                      NAS_MMC_RcvGmmPsRegResultInd_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_CIPHER_INFO_IND,
                      NAS_MMC_RcvRrMmCipherInfoInd_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      GMMMMC_CIPHER_INFO_IND,
                      NAS_MMC_RcvGmmCipherInfoInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_OM_MAINTAIN_INFO_IND,
                      NAS_MMC_RcvTafOmMaintainInfoInd_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_GPRS_SERVICE_IND,
                      NAS_MMC_RcvGmmGprsServiceInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_SIGN_REPORT_REQ,
                      NAS_MMC_RcvMsccSignReportReq_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_SIM_AUTH_FAIL_IND,
                      NAS_MMC_RcvMmSimAuthFail_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_SIM_AUTH_FAIL_IND,
                      NAS_MMC_RcvGmmSimAuthFail_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_CM_SERVICE_IND,
                      NAS_MMC_RcvMmCmServiceInd_PreProc),

#if   (FEATURE_ON == FEATURE_LTE)
    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_CSFB_ABORT_IND,
                      NAS_MMC_RcvMmCsfbAbortInd_PreProc),
#endif

    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_PLMN_SEARCH_IND,
                      NAS_MMC_RcvMmPlmnSearchInd_PreProc),

#if (FEATURE_MULTI_MODEM == FEATURE_ON)
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_OTHER_MODEM_INFO_NOTIFY,
                      NAS_MMC_RcvMsccOtherModemInfoNotify_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_OTHER_MODEM_DPLMN_NPLMN_INFO_NOTIFY,
                      NAS_MMC_RcvMsccOtherModemDplmnNplmnInfoNotify_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_NCELL_INFO_NOTIFY,
                      NAS_MMC_RcvMsccNcellInfoNotify_PreProc),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PS_TRANSFER_NOTIFY,
                      NAS_MMC_RcvMsccPsTransferInd_PreProc),
#endif

    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMCMM_CM_SERVICE_REJECT_IND,
                      NAS_MMC_RcvCmServiceRejectInd_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIOD_DELETE_DISABLED_PLMN_WITH_RAT_TIMER,
                      NAS_MMC_RcvPeriodDeleteDisabledPlmnWithRatExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_FORBID_LA_TIMER,
                      NAS_MMC_RcvForbidLaTimerExpired_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_CUSTOMIZED_FORB_LA_TIMER,
                      NAS_MMC_RcvCustomizedForbLaTimerExpired_PreProc),

#if   (FEATURE_ON == FEATURE_LTE)
    NAS_ACT_TBL_ITEM( WUEPS_PID_MM,
                      MMMMC_ABORT_IND,
                      NAS_MMC_RcvMmAbortInd_PreProc),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_WAIT_ENABLE_LTE_TIMER,
                      NAS_MMC_RcvEnableLteExpired_PreProc ),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_T3402_LEN_NOTIFY,
                      NAS_MMC_RcvLmmT3402LenNotify_PreProc),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_EUTRAN_NOT_ALLOW_NOTIFY,
                      NAS_MMC_RcvLmmEutranNotAllowNotify_PreProc),
#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CL_ASSOCIATED_INFO_NTF,
                      NAS_MMC_RcvMsccCLAssociatedInfoNtf_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CL_INTERSYS_START_NTF,
                      NAS_MMC_RcvMsccCLInterSysStartNtf_PreProc),


    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CURR_GEO_INFO_NTF,
                      NAS_MMC_RcvMsccCurrGeoInfoNtf_PreProc),
#endif

#endif
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_EPLMN_INFO_NTF,
                      NAS_MMC_RcvMsccEplmnInfoNtf_PreProc),

#if (FEATURE_ON == FEATURE_PTM)
    NAS_ACT_TBL_ITEM( MSP_PID_DIAG_APP_AGENT,
                      ID_OM_ERR_LOG_REPORT_REQ,
                      NAS_MMC_RcvAcpuOmErrLogRptReq_PreProc),

    NAS_ACT_TBL_ITEM( MSP_PID_DIAG_APP_AGENT,
                      ID_OM_ERR_LOG_CTRL_IND,
                      NAS_MMC_RcvAcpuOmErrLogCtrlInd_PreProc),

    NAS_ACT_TBL_ITEM( MSP_PID_DIAG_APP_AGENT,
                      ID_OM_FTM_CTRL_IND,
                      NAS_MMC_RcvAcpuOmFtmCtrlInd_PreProc),

#endif

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_NET_SCAN_REQ,
                      NAS_MMC_RcvMsccNetScanReq_PreProc ),

    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_PERIODIC_NET_SCAN_TIMER,
                      NAS_MMC_RcvPeriodicNetScanExpired_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_NET_SCAN_CNF,
                      NAS_MMC_RcvRrMmNetScanCnf_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_NET_SCAN_CNF,
                      NAS_MMC_RcvRrMmNetScanCnf_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_ABORT_NET_SCAN_REQ,
                      NAS_MMC_RcvMsccAbortNetScanReq_PreProc ),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_NET_SCAN_STOP_CNF,
                      NAS_MMC_RcvRrMmNetScanStopCnf_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_NET_SCAN_STOP_CNF,
                      NAS_MMC_RcvRrMmNetScanStopCnf_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_NCELL_MONITOR_IND,
                      NAS_MMC_RcvGasNcellMonitorInd_PreProc),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_IMS_VOICE_CAP_NOTIFY,
                      NAS_MMC_RcvMsccImsVoiceCapInd_PreProc),

     NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      GMMMMC_SERVICE_REQUEST_RESULT_IND,
                      NAS_MMC_RcvGmmServiceRequestResultInd_PreProc),


    NAS_ACT_TBL_ITEM( WUEPS_PID_GMM,
                      MMCGMM_SIGNALING_STATUS_IND,
                      NAS_MMC_RcvGmmSigStateInd_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_AREA_LOST_IND,
                      NAS_MMC_RcvWasAreaLostInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_AREA_LOST_IND,
                      NAS_MMC_RcvGasAreaLostInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_RRM,
                      ID_RRM_PS_STATUS_IND,
                      NAS_MMC_RcvRrmPsStatusInd_PreProc),
    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_SUSPEND_IND,
                      NAS_MMC_RcvRrmmSuspendInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_SUSPEND_IND,
                      NAS_MMC_RcvRrmmSuspendInd_PreProc),

    NAS_ACT_TBL_ITEM( WUEPS_PID_WRR,
                      RRMM_RESUME_IND,
                      NAS_MMC_RcvRrmmResumeInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_GAS,
                      RRMM_RESUME_IND,
                      NAS_MMC_RcvRrmmResumeInd_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_SRV_ACQ_REQ,
                      NAS_MMC_RcvMsccSrvAcqReq_PreProc),

#if (FEATURE_ON == FEATURE_DSDS)
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_BEGIN_SESSION_NOTIFY,
                      NAS_MMC_RcvMsccBeginSessionNotify_PreProc),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_END_SESSION_NOTIFY,
                      NAS_MMC_RcvMsccEndSessionNotify_PreProc),

#endif

#if   (FEATURE_ON == FEATURE_LTE)

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_SUSPEND_IND,
                      NAS_MMC_RcvLmmSuspendInd_PreProc ),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_RESUME_IND,
                      NAS_MMC_RcvLmmResumeInd_PreProc ),

    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_INFO_CHANGE_NOTIFY,
                      NAS_MMC_RcvLmmInfoChangeNotifyInd_PreProc ),

#endif

#if (FEATURE_ON == FEATURE_IMS)
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_IMS_SRV_INFO_NOTIFY,
                      NAS_MMC_RcvMsccImsSrvInfoNotify_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_IMS_SWITCH_STATE_IND,
                      NAS_MMC_RcvMsccImsSwitchStateInd_PreProc ),

#endif

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_VOICE_DOMAIN_CHANGE_IND,
                      NAS_MMC_RcvMsccVoiceDomainChangeInd_PreProc ),

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CFPLMN_SET_REQ,
                      NAS_MMC_RcvMsccCFPlmnSetReq_PreProc),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_CFPLMN_QUERY_REQ,
                      NAS_MMC_RcvMsccCFPlmnQueryReq_PreProc),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_SDT_CONNECTED_IND,
                      NAS_MMC_RcvTafSDTConnInd_PreProc),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PREF_PLMN_QUERY_REQ,
                      NAS_MMC_RcvMsccPrefPlmnQueryReq_PreProc ),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PREF_PLMN_SET_REQ,
                      NAS_MMC_RcvMsccPrefPlmnSetReq_PreProc ),
    NAS_ACT_TBL_ITEM( WUEPS_PID_USIM,
                      USIMM_UPDATEFILE_CNF,
                      NAS_MMC_RcvUsimSetFileCnf_PreProc),
    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_WAIT_USIM_SET_FILE_CNF,
                      NAS_MMC_WaitUsimSetFileExpired_PreProc ),

#if (FEATURE_ON == FEATURE_PTM)
    NAS_ACT_TBL_ITEM( UEPS_PID_MTA,
                      ID_MTA_MMC_GET_NAS_CHR_INFO_REQ,
                      NAS_MMC_RcvMtaGetNasChrInfoReq_PreProc ),
#endif

    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_PLMN_PRI_CLASS_QUERY_REQ,
                      NAS_MMC_RcvMsccPlmnPriClassQueryReq_PreProc ),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_AUTO_RESEL_SET_REQ,
                      NAS_MMC_RcvMsccAutoReselSetReq_PreProc ),

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA) && (FEATURE_ON == FEATURE_LTE)
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_BG_SEARCH_REQ,
                      NAS_MMC_RcvMsccBgSearchReq_PreProc),
    NAS_ACT_TBL_ITEM( UEPS_PID_MSCC,
                      ID_MSCC_MMC_STOP_BG_SEARCH_REQ,
                      NAS_MMC_RcvMsccStopBgSearchReq_PreProc),
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_BG_SEARCH_HRPD_CNF,
                      NAS_MMC_RcvLmmBgSearchHrpdCnf_PreProc),
    NAS_ACT_TBL_ITEM( PS_PID_MM,
                      ID_LMM_MMC_STOP_BG_SEARCH_HRPD_CNF,
                      NAS_MMC_RcvLmmStopBgSearchHrpdCnf_PreProc),
    NAS_ACT_TBL_ITEM( VOS_PID_TIMER,
                      TI_NAS_MMC_WAIT_LMM_BG_SEARCH_HRPD_CNF,
                      NAS_MMC_WaitLmmBgSearchHrpdExpired_PreProc),
#endif
};

/*lint -e553*/
/* 不进状态机处理的消息 状态表 */
NAS_STA_STRU        g_astNasMmcPreProcessFsmTbl[]   =
{
    NAS_STA_TBL_ITEM( NAS_MMC_L1_STA_PREPROC,
                      g_astNasMmcPreProcessActTbl ),
};


VOS_UINT32 NAS_MMC_GetPreProcessStaTblSize( VOS_VOID  )
{
    return (sizeof(g_astNasMmcPreProcessFsmTbl)/sizeof(NAS_STA_STRU));
}
/*lint +e553*/


NAS_FSM_DESC_STRU * NAS_MMC_GetPreFsmDescAddr(VOS_VOID)
{
    return (&g_stNasMmcPreFsmDesc);
}




/*****************************************************************************
  3 函数实现
*****************************************************************************/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
