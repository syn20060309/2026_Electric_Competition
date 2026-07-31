#include "app_irtracking.h"

#include "BSP/Mode/car_mode.h"
#include "lap_finish.h"
#include "race_control.h"
#include "race_timer.h"
#include "timer.h"
#include "tracking_i2c_policy.h"
#include "tracking_sample.h"

#define IRTrack_Trun_KP (250)
#define IRTrack_Trun_KI (0.2) 
#define IRTrack_Trun_KD (2) 
#define CHANGE_THRESHOLD 3
#define IR_I2C_TIMEOUT_MS 5U
#define IR_I2C_LOOP_GUARD 100000U
const float pid_out_max = 5000.0f; 
const float Integral_max = 500.0f; // 积分限幅值 
int pid_output_IRR = 0;
u8 trun_flag = 0;


float PID_IR_Calc(int16_t actual_value)
{
    float pid_out = 0;
    int16_t error; 
    static int16_t error_last = 0; //上次的误差初始为0  Last error
    static float Integral = 0; // 初始化积分项 Initialize integral term

    error = actual_value;
//    if(err == 0)          
//    {
//        Integral = 0;          //积分清零   Integral cleared
//    }
    Integral += error;           // 更新积分项，并进行限幅 Update the integral term and limit it
    if (Integral > Integral_max) Integral = Integral_max;               //积分限幅 Integral limiting
    if (Integral < -Integral_max) Integral = -Integral_max;             //积分限幅 Integral limiting

    // 位置式 PID
    pid_out = error * IRTrack_Trun_KP
              + IRTrack_Trun_KI * Integral
              + (error - error_last) * IRTrack_Trun_KD;

    error_last = error;       // 更新积分项，并进行限幅 Update the integral term and limit it

    // 对输出进行限幅Output limiting value
    if (pid_out > pid_out_max) pid_out = pid_out_max;  
    if (pid_out < -pid_out_max) pid_out = -pid_out_max;

    return pid_out;
}
 
void IRI2C_WriteByte(uint8_t addr, uint8_t dat) {
    uint8_t temp[2];
    temp[0] = addr;
    temp[1] = dat;
 
    DL_I2C_fillControllerTXFIFO(Sensor_INST, temp, 2);
    while (!(DL_I2C_getControllerStatus(Sensor_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
 
    DL_I2C_startControllerTransfer(Sensor_INST, IR_I2C_ADDR, DL_I2C_CONTROLLER_DIRECTION_TX, 2);
    while (DL_I2C_getControllerStatus(Sensor_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    while (!(DL_I2C_getControllerStatus(Sensor_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    DL_I2C_flushControllerTXFIFO(Sensor_INST);
}
 
static void IRI2C_Abort(void)
{
    DL_I2C_resetControllerTransfer(Sensor_INST);
    DL_I2C_flushControllerTXFIFO(Sensor_INST);
    DL_I2C_flushControllerRXFIFO(Sensor_INST);
    DL_I2C_clearInterruptStatus(Sensor_INST,
        DL_I2C_INTERRUPT_CONTROLLER_NACK |
        DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST);
}

static bool IRI2C_DeadlineExpired(
    uint32_t start_ms, uint32_t *loop_guard)
{
    if (*loop_guard == 0U) {
        return true;
    }

    (*loop_guard)--;
    return (uint32_t) (Get_Time() - start_ms) >= IR_I2C_TIMEOUT_MS;
}

static bool IRI2C_WaitForIdle(void)
{
    uint32_t start_ms = Get_Time();
    uint32_t loop_guard = IR_I2C_LOOP_GUARD;

    while (true) {
        uint32_t status = DL_I2C_getControllerStatus(Sensor_INST);

        if (Tracking_I2CStatusFailed(status)) {
            IRI2C_Abort();
            return false;
        }
        if (Tracking_I2CStatusReady(status, false)) {
            return true;
        }
        if (IRI2C_DeadlineExpired(start_ms, &loop_guard)) {
            IRI2C_Abort();
            return false;
        }
    }
}

static bool IRI2C_WaitForTransfer(void)
{
    uint32_t start_ms = Get_Time();
    uint32_t loop_guard = IR_I2C_LOOP_GUARD;

    while (true) {
        uint32_t status = DL_I2C_getControllerStatus(Sensor_INST);

        if (Tracking_I2CStatusFailed(status)) {
            IRI2C_Abort();
            return false;
        }
        if (Tracking_I2CStatusReady(status, true)) {
            return true;
        }
        if (IRI2C_DeadlineExpired(start_ms, &loop_guard)) {
            IRI2C_Abort();
            return false;
        }
    }
}

bool IRI2C_ReadByte(uint8_t addr, uint8_t *data)
{
    if (data == NULL) {
        return false;
    }

    if (!IRI2C_WaitForIdle()) {
        return false;
    }

    DL_I2C_flushControllerTXFIFO(Sensor_INST);
    DL_I2C_flushControllerRXFIFO(Sensor_INST);
    (void) DL_I2C_fillControllerTXFIFO(Sensor_INST, &addr, 1U);
    DL_I2C_disableInterrupt(
        Sensor_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    DL_I2C_startControllerTransfer(Sensor_INST, IR_I2C_ADDR,
        DL_I2C_CONTROLLER_DIRECTION_TX, 1U);

    if (!IRI2C_WaitForTransfer()) {
        return false;
    }

    DL_I2C_startControllerTransfer(Sensor_INST, IR_I2C_ADDR,
        DL_I2C_CONTROLLER_DIRECTION_RX, 1U);
    if (!IRI2C_WaitForTransfer() ||
        DL_I2C_isControllerRXFIFOEmpty(Sensor_INST)) {
        IRI2C_Abort();
        return false;
    }

    *data = DL_I2C_receiveControllerData(Sensor_INST);
    return true;
}



bool deal_IRdata(
    u8 *x1, u8 *x2, u8 *x3, u8 *x4,
    u8 *x5, u8 *x6, u8 *x7, u8 *x8)
{
    u8 IRbuf;

    if ((x1 == NULL) || (x2 == NULL) || (x3 == NULL) || (x4 == NULL) ||
        (x5 == NULL) || (x6 == NULL) || (x7 == NULL) || (x8 == NULL) ||
        !IRI2C_ReadByte(0x30U, &IRbuf)) {
        return false;
    }
	
	*x1 = (IRbuf>>7)&0x01;
	*x2 = (IRbuf>>6)&0x01;
	*x3 = (IRbuf>>5)&0x01;
	*x4 = (IRbuf>>4)&0x01;
	*x5 = (IRbuf>>3)&0x01;
	*x6 = (IRbuf>>2)&0x01;
	*x7 = (IRbuf>>1)&0x01;
	*x8 = (IRbuf>>0)&0x01;
    return true;
}


void printf_i2c_data(void)
{
    static uint8_t ir_x1,ir_x2,ir_x3,ir_x4,ir_x5,ir_x6,ir_x7,ir_x8;
    if (deal_IRdata(
            &ir_x1, &ir_x2, &ir_x3, &ir_x4,
            &ir_x5, &ir_x6, &ir_x7, &ir_x8)) {
        printf("x1:%d,x2:%d,x3:%d,x4:%d,x5:%d,x6:%d,x7:%d,x8:%d\r\n",
            ir_x1, ir_x2, ir_x3, ir_x4, ir_x5, ir_x6, ir_x7, ir_x8);
    } else {
        printf("IR I2C read failed\r\n");
    }
}
//void LineWalking(void)
//{
//    static int8_t err = 0;
//    static u8 x1,x2,x3,x4,x5,x6,x7,x8;
//    
//    deal_IRdata(&x1,&x2,&x3,&x4,&x5,&x6,&x7,&x8);
//    
//    // 急弯检测 - 优先处理
//    if(x1 == 0 && x2 == 0 && x3 == 0) {
//        err = -4;  // 左急弯
//    }
//    else if(x6 == 0 && x7 == 0 && x8 == 0) {
//        err = 4;   // 右急弯
//    }
//    // 大角度转弯检测
//    else if(x1 == 0 && x2 == 0) {
//        err = -3;  // 左大弯
//    }
//    else if(x7 == 0 && x8 == 0) {
//        err = 3;   // 右大弯
//    }
//    // 中线检测
//    else if(x4 == 0 || x5 == 0) {
//        err = 0;   // 直行
//    }
//    // 轻微偏左
//    else if(x3 == 0) {
//        err = -1;
//    }
//    // 轻微偏右
//    else if(x6 == 0) {
//        err = 1;
//    }
//    // 中度偏左
//    else if(x2 == 0) {
//        err = -2;
//    }
//    // 中度偏右
//    else if(x7 == 0) {
//        err = 2;
//    }
//    // 极端偏左
//    else if(x1 == 0) {
//        err = -3;
//    }
//    // 极端偏右
//    else if(x8 == 0) {
//        err = 3;
//    }
//    // 丢失黑线 - 原地旋转搜索
//    else {
//     err =0;
//    }

//    pid_output_IRR = (int)(PID_IR_Calc(err));
//    Motion_Car_Control(IRR_SPEED, 0, pid_output_IRR);
//}
#if LAP_FINISH_DEBUG
static void Tracking_DebugLapSample(uint32_t now_ms, uint32_t elapsed_ms,
    bool sensor_valid, uint8_t active_mask, LapFinish_Event event)
{
    static uint32_t last_report_ms;
    static bool last_finish_pattern;
    LapFinish_State state = LapFinish_GetState();
    bool enabled = LapFinish_StartLineCleared() &&
        (elapsed_ms >= FINISH_MIN_TIME_MS);
    uint8_t active_count = Tracking_CountActive(active_mask);
    bool finish_pattern = sensor_valid &&
        (active_count >= FINISH_ACTIVE_COUNT_THRESHOLD);

    if (finish_pattern && !last_finish_pattern) {
        printf("LAP FINISH-CANDIDATE time=%lu mask=0x%02X count=%u\r\n",
            (unsigned long) elapsed_ms, active_mask, active_count);
    }
    last_finish_pattern = finish_pattern;

    if (event == LAP_FINISH_EVENT_FINISH) {
        printf("LAP FINISH TRIGGER time=%lu\r\n",
            (unsigned long) elapsed_ms);
    }

    if ((event == LAP_FINISH_EVENT_NONE) &&
        ((uint32_t) (now_ms - last_report_ms) < 100U)) {
        return;
    }

    last_report_ms = now_ms;
    printf("LAP state=%d time=%lu valid=%u mask=0x%02X count=%u "
           "threshold=%u cleared=%u enabled=%u confirm=%u event=%d\r\n",
        (int) state, (unsigned long) elapsed_ms, sensor_valid ? 1U : 0U,
        active_mask, active_count, FINISH_ACTIVE_COUNT_THRESHOLD,
        LapFinish_StartLineCleared() ? 1U : 0U, enabled ? 1U : 0U,
        LapFinish_GetConfirmCount(), (int) event);
}
#endif

void LineWalking(void)
{
	static int8_t err = 0;
	static u8 x1,x2,x3,x4,x5,x6,x7,x8;
    uint16_t current_speed;
    bool sensor_valid;
    uint8_t active_mask = 0U;
    uint32_t now_ms;
    uint32_t elapsed_ms;
    LapFinish_Event lap_event;

    now_ms = Get_Time();
    elapsed_ms = RaceTimer_GetElapsedMs();
    if (LapFinish_CheckTimeout(elapsed_ms)) {
#if LAP_FINISH_DEBUG
        printf("LAP TIMEOUT TRIGGER time=%lu\r\n",
            (unsigned long) elapsed_ms);
#endif
        Car_TimeoutStop();
        return;
    }

    sensor_valid = deal_IRdata(
        &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8);
    if (sensor_valid) {
        active_mask = Tracking_BuildActiveMask(
            x1, x2, x3, x4, x5, x6, x7, x8);
    }

    lap_event = LapFinish_Update(
        now_ms, elapsed_ms, sensor_valid, active_mask);

#if LAP_FINISH_DEBUG
    Tracking_DebugLapSample(
        now_ms, elapsed_ms, sensor_valid, active_mask, lap_event);
#endif

    if (lap_event == LAP_FINISH_EVENT_FINISH) {
        Car_FinishStop();
        return;
    }
    if (!sensor_valid) {
        return;
    }
	
	//debug
//	static char bufbuf[30]={'\0'};
//	sprintf(bufbuf,"%d\t %d\t %d\t %d\t %d\t %d\t %d\t %d\t \r\n",x1,x2,x3,x4,x5,x6,x7,x8);
//	uart0_send_string((char*)bufbuf);
	
    //优先判断	Priority judgment
//	if((x1 == 0 && x2 == 0 && x3 == 0) || 
//	(x6 == 0 && x7 == 0 && x8 == 0)) {
//	 err = (x1 == 0) ? -4 : 4;  // 根据急弯方向调整偏差值
// }
	if(x4 == 0||x5 == 0) {
		err = 0;  // 中线传感器检测到黑线，直行
	}
  	else if(x1 == 1 && x2 == 1  && x3 == 1&& x4 == 0 && x5 == 1 && x6 == 1  && x7 == 1 && x8 == 1) // 1110 1111
	{
		err = -1;
	}
	else if(x1 == 1 && x2 == 1  && x3 == 0&& x4 == 0 && x5 == 1 && x6 == 1  && x7 == 1 && x8 == 1) // 1100 1111
	{
		err = -2;
	}
	else if(x1 == 1 && x2 == 1  && x3 == 0&& x4 == 1 && x5 == 1 && x6 == 1  && x7 == 1 && x8 == 1) // 1101 1111
	{
		err = -2;
	}
	else if(x1 == 1 && x2 == 0  && x3 == 0&& x4 == 1 && x5 == 1 && x6 == 1  && x7 == 1 && x8 == 1) // 1001 1111
	{
		err = -3;
	}
	
	else if(x1 == 1 && x2 == 1  && x3 == 1&& x4 == 1 && x5 == 0 && x6 == 1  && x7 == 1 && x8 == 1) // 1111 0111
	{
		err = 1;
	} 
	else if(x1 == 1 && x2 == 1  && x3 == 1&& x4 == 1 && x5 == 0 && x6 == 0  && x7 == 1 && x8 == 1) // 1111 0011
	{
		err = 2;
	}
	else if(x1 == 1 && x2 == 1  && x3 == 1&& x4 == 1 && x5 == 1 && x6 == 0  && x7 == 1 && x8 == 1) // 1111 1011
	{
		err = 2;
	}
	else if(x1 == 1 && x2 == 1  && x3 == 1&& x4 == 1 && x5 == 1 && x6 == 0  && x7 == 0 && x8 == 1) // 1111 1001
	{
		err = 3;
	}
	else if(x3 == 0) {
		err = -1;  // 左偏
	}
	else if(x2 == 0) {
		err = -2;  // 更左偏
	}
	else if(x7 == 0) {
		err = 4;   // 更右偏
	}

	//剩下的就保持上一个状态	The rest will remain in the previous state
	pid_output_IRR = (int)(PID_IR_Calc(err));
	current_speed = CarMode_GetSpeed();
	if (current_speed == 0U) {
		Contrl_Pwm(0, 0, 0, 0);
		return;
	}

	Motion_Car_Control((int16_t) current_speed, 0, pid_output_IRR);

}

//检测现在位于黑线还是在白线上	Detection is now on the black line or on the white line
int LineCheck(void)
{
    static u8 x1,x2,x3,x4,x5,x6,x7,x8;
	if (!deal_IRdata(
            &x1, &x2, &x3, &x4, &x5, &x6, &x7, &x8)) {
        return WHITE;
    }
	
	// 只有当所有传感器都为1（未检测到黑线）时，if_have才为0
	if(x1 && x2 && x3 && x4 && x5 && x6 && x7 && x8)
	{
		return WHITE;
	}
	else
	{
		return BLACK;
	}
		
}


