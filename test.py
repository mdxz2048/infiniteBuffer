import subprocess
import platform
import time

# 配置
cpu = platform.processor()  # 获取CPU型号
system = platform.system()  # 获取系统版本
progress_send_delay_list = ["1", "10", "100", "1000"]  # 发送端包间延时us
progress_recv_thread_count_list = ["1", "2", "4", "6", "8"]  # 接收端线程个数
total_test_messages = 1000  # 每个测试的消息总数

# 结果文件
output_file = "test_results.csv"
with open(output_file, "w") as file:
    file.write("CPU, system, 发送端包间延时us, 发送进程发送消息总数, 接收端线程个数, 接收端消息接收总数, 丢包率%\n")

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
        receiver_cmd = f"./debug/bin/test_processRecv --receive_thread_count {thread_count}"
        sender_cmd = f"./debug/bin/test_processSend --message_delay_us {delay} --total_messages {total_test_messages}"
        # 启动接收端和发送端进程
        print("接收端命令:", receiver_cmd)
        print("发送端命令:", sender_cmd)

        receiver_process = subprocess.Popen(receiver_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        time.sleep(1)
        sender_process = subprocess.Popen(sender_cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)



        # 等待进程结束并捕获输出
        sender_output, sender_errors = sender_process.communicate()
        receiver_output, receiver_errors = receiver_process.communicate()

        # 解析输出
        sent_messages = parse_flagged_output(sender_output, "[PYTHON_RESULT_FLAG]  TotalMessagesSent")
        received_messages = parse_flagged_output(receiver_output, "[PYTHON_RESULT_FLAG]  TotalMessagesRecv")

        # 计算丢包率
        packet_loss_rate = (1 - received_messages / sent_messages) * 100 if sent_messages > 0 else 0

        # 记录结果到文件
        with open(output_file, "a") as file:
            file.write(f"{cpu}, {system}, {delay}, {total_test_messages}, {thread_count}, {received_messages}, {packet_loss_rate:.2f}\n")
        print(f"测试完成：发送 {sent_messages}, 接收 {received_messages}, 丢包率 {packet_loss_rate:.2f}%")

print("测试完成，结果已记录到", output_file)
