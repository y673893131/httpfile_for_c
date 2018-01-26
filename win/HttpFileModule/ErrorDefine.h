#pragma once

enum ERROR_ENUM
{
	ERROR_SUCCESS_DEFAULT=0x00000000,					/* 成功 */
	ERROR_INPUT_PARAM=0x00000001,						/* 输入参数错误 */
	ERROR_REMOTE_FILE_NOT_FOUND,						/* url 文件不存在 */
	ERROR_LOCAL_FILE_NOT_FOUND,							/* 本地文件不存在 */
	ERROR_LOCAL_FILE_CAN_NOT_ACCESS,					/* 本地文件没有访问权限 */
	ERROR_LAST_TASK_IS_WORKING,							/* 上一个任务正在工作 */
	ERROR_CREATE_TASK_THREAD_FAILED,					/* 创建任务线程失败 */
	ERROR_ANALYSIS_URL,									/* 解析URL失败 */
	ERROR_GET_HTTP_CONNECTION,							/* 获取http连接失败 */
	ERROR_QUARY_CONTENT_LENGTH,							/* 查询http文件长度为0 */
	ERROR_HTTP_EXCEPTION,								/* http 异常，通过GetLastError()，获取对应错误码 */
};
