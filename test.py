import subprocess
import platform
import time

# 配置
cpu = platform.processor()  # 获取CPU型号
system = platform.system()  # 获取系统版本
progress_send_delay_list = ["1", "10", "100", "1000", "10000", "100000"]  # 发送端包间延时us
progress_recv_thread_count_list = ["1", "2", "4", "6", "8"]  # 接收端线程个数
total_test_messages = 10000  # 每个测试的消息总数

# 结果文件
output_file = "test_results.csv"
with open(output_file, "w", encoding="utf-8") as file:
    file.write("CPU, system, progressSendDelay(us),progressSendTotalMessages, progressRecvThreadsCnt,progressRecvTotalMessages, lossRate%\n")

# 解析带有特定标志的输出行
def parse_flagged_output(output, flag):
    for line in output.decode().splitlines():
        if flag in line:
            return int(line.split('=')[1])
    return 0

# 测试循环
for delay in progress_send_delay_list:
    for thread_count in progress_recv_thread_count_list:
        print(f"开始测试：延时 {delay}us, 接收线程数 {thread_count}")
        # 构建命令
        receiver_cmd = ["./debug/bin/test_processRecv", "--receive_thread_count", str(thread_count)]
        sender_cmd = ["./debug/bin/test_processSend", "--message_delay_us", str(delay), "--total_messages", str(total_test_messages)]

        # 启动接收端进程，忽略stdout，只获取stderr
        receiver_process = subprocess.Popen(
            receiver_cmd, 
            stdout=subprocess.DEVNULL, 
            stderr=subprocess.PIPE
        )
        
        # 稍作延迟，然后启动发送端进程，同样忽略stdout
        time.sleep(1)
        sender_process = subprocess.Popen(
            sender_cmd, 
            stdout=subprocess.DEVNULL, 
            stderr=subprocess.PIPE
        )

        # 等待进程结束并捕获标准错误
        _, receiver_errors = receiver_process.communicate()
        _, sender_errors = sender_process.communicate()

        # 解析输出
      # 解析输出
        received_messages = parse_flagged_output(receiver_errors, "[PYTHON_RESULT_FLAG]  TotalMessagesRecv")
        sent_messages = total_test_messages  # 假设发送端总是发送完所有消息
        # 计算丢包率
        packet_loss_rate = (1 - received_messages / sent_messages) * 100 if sent_messages > 0 else 0

        # 记录结果到文件
        with open(output_file, "a") as file:
            file.write(f"{cpu}, {system}, {delay}, {total_test_messages}, {thread_count}, {received_messages}, {packet_loss_rate:.2f}\n")
        print(f"测试完成：发送 {sent_messages}, 接收 {received_messages}, 丢包率 {packet_loss_rate:.2f}%")

print("测试完成，结果已记录到", output_file)
