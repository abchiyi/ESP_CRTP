/**    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie控制固件
 *
 * 版权所有 (C) 2011-2012 Bitcraze AB
 *
 * 本程序是自由软件：您可以依据GNU通用公共许可证版本3对其重新分发和/或修改。
 *
 * 本程序是免费分发的，希望它有用，
 * 但不提供任何保证；不提供关于适销性或特定用途适用性的默示保证。有关更多详细信息，请参阅GNU通用公共许可证。
 *
 * 您应该已经随本程序一起收到了GNU通用公共许可证。
 * 如果没有，请查看 <http://www.gnu.org/licenses/>。
 *
 * crtp.h - CrazyRealtimeTransferProtocol堆栈
 */

#ifndef CRTP_H_
#define CRTP_H_

#include <stdint.h>
#include <stdbool.h>

#define CRTP_MAX_DATA_SIZE 30

#define CRTP_HEADER(port, channel) (((port & 0x0F) << 4) | (channel & 0x0F))

#define CRTP_IS_NULL_PACKET(P) ((P.header & 0xF3) == 0xF3)

typedef enum
{
  CRTP_PORT_CONSOLE = 0x00,          //  Console 使用 consoleprintf 函数将调试信息输出到 PC 端。
  CRTP_PORT_PARAM = 0x02,            // 读写 Crazyflie 参数。参数可在源码中用宏表示。
  CRTP_PORT_SETPOINT = 0x03,         // 发送 roll/pitch/yaw/thrust 控制指令。
  CRTP_PORT_MEM = 0x04,              // 访问非易失性存储，如 1 线访问和 I2C 访问。仅支持 Crazyflie 2.0
  CRTP_PORT_LOG = 0x05,              // 设置日志变量。日志变量将定期发送至 Crazyflie，日志变量在 Crazyflie 源码中用宏表示。
  CRTP_PORT_LOCALIZATION = 0x06,     // 定位端口，用于接收和处理定位数据
  CRTP_PORT_SETPOINT_GENERIC = 0x07, // 本地化相关包
  CRTP_PORT_PLATFORM = 0x0D,         // 用于 misc platform 控制，如调试和掉电等
  CRTP_PORT_LINK = 0x0F              // 用于控制和访问通信链路层
} CRTPPort;

typedef struct _CRTPPacket
{
  uint8_t size; //< 数据大小
  union
  {
    struct
    {
      union
      {
        uint8_t header; //< 选择通道和端口的头部
        struct
        {
#ifndef CRTP_HEADER_COMPAT
          uint8_t channel : 2; //< 在端口内选择的通道
          uint8_t reserved : 2;
          uint8_t port : 4; //< 选择的端口
#else
          uint8_t channel : 2;
          uint8_t port : 4;
          uint8_t reserved : 2;
#endif
        };
      };
      uint8_t data[CRTP_MAX_DATA_SIZE]; //< 数据
    };
    uint8_t raw[CRTP_MAX_DATA_SIZE + 1]; //< 完整的数据包 "原始"
  };
} __attribute__((packed)) CRTPPacket;

typedef void (*CrtpCallback)(CRTPPacket *);

/**
 * 初始化CRTP堆栈
 */
void crtpInit(void);

bool crtpTest(void);

/**
 * 初始化任务的队列和调度。
 *
 * @param[in] taskId CRTP任务的ID
 */
void crtpInitTaskQueue(CRTPPort taskId);

/**
 * 注册一个回调函数，用于特定端口。
 *
 * @param[in] port 设置回调的Crtp端口
 * @param[in] cb 当在'port'上接收到数据包时将调用的回调函数
 *
 * @note 每个端口只能注册一个回调函数！最后注册的回调函数将被调用
 */
void crtpRegisterPortCB(int port, CrtpCallback cb);

/**
 * 将数据包放入TX任务中
 *
 * 如果TX堆栈满了，最旧的低优先级数据包将被丢弃
 *
 * @param[in] p 要发送的CRTPPacket
 */
int crtpSendPacket(CRTPPacket *p);

/**
 * 将数据包放入TX任务中
 *
 * 如果TX堆栈满了，函数将阻塞直到有空位（适用于控制台实现）
 */
int crtpSendPacketBlock(CRTPPacket *p);

/**
 * 使用指定的端口获取数据包。
 *
 * @param[in]  portId CRTP端口ID
 * @param[out] p      包含信息的CRTP数据包（如果没有可获取的数据包，则不变）
 *
 * @returns 从队列中获取的状态
 */
int crtpReceivePacket(CRTPPort portId, CRTPPacket *p);

/**
 * 使用指定的端口ID获取数据包。
 *
 * @param[in]  portId CRTP数据端口
 * @param[out] p      包含信息的CRTP数据包（如果没有可获取的数据包，则不变）
 * @param[in] wait    等待时间（毫秒）
 *
 * @returns 从队列中获取的状态
 */
int crtpReceivePacketWait(CRTPPort portId, CRTPPacket *p, int wait);

/**
 * 获取队列中TX数据包数量
 *
 * @return 数据包的数量
 */
int crtpGetFreeTxQueuePackets(void);

/**
 * 等待指定任务ID的数据包到达
 *
 * @param[in]  taskId CRTP任务的ID
 * @paran[out] p      包含信息的数据包
 *
 * @return 从队列中获取的状态
 */
int crtpReceivePacketBlock(CRTPPort taskId, CRTPPacket *p);

/**
 * 函数指针结构，由CRTP链接填充以允许CRTP使用多种链接
 */
struct crtpLinkOperations
{
  int (*setEnable)(bool enable);
  int (*sendPacket)(CRTPPacket *pk);
  int (*receivePacket)(CRTPPacket *pk);
  bool (*isConnected)(void);
  int (*reset)(void);
};

void crtpSetLink(struct crtpLinkOperations *lk);

/**
 * 检查连接超时是否已达到，否则
 * 我们将假设我们已连接。
 *
 * @return 如果连接，则为true，否则为false
 */
bool crtpIsConnected(void);

/**
 * 通过刷新包含包的所有队列来重置CRTP通信。
 *
 * @return 成功返回0
 */
int crtpReset(void);

#endif /*CRTP_H_*/
