修改时间：2024-05-16
	1.增加任务创建例程模板
	2.打印测试
	创建任务函数原型及其参数说明
	BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
				const char * const pcName,
				const configSTACK_DEPTH_TYPE usStackDepth,
				void * const pvParameters,
				UBaseType_t uxPriority,
				TaskHandle_t * const pxCreatedTask )

pxTaskCode:任务函数的名称
pcName：定义一个能看到的任务名称，为字符串，可以与任务函数不同。后续可以通过xTaskGetHandle()函数来获取到这个函数的句柄。长度定义在configMAX_TASK_NAME_LEN，默认是16字节
usStackDepth：定义栈空间大小，configSTACK_DEPTH_TYPE默认定义的是uint16_t，也就是说，当usStackDepth=100实际的堆栈空间大小为200字节。usStackDepth最小值为configMINIMAL_STACK_SIZE，默认定义是128。所以每个任务堆栈最小为256字节
pvParameters：传递给任务的参数
uxPriority：任务优先级，范围 0~ configMAX_PRIORITIES-1。越大越高
pxCreatedTask ：任务句柄

返回值
0：pdPASS创建成功
-1：errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY堆内存不足


任务创建相关函数
xTaskCreate() 动态创建一个任务
xTaskCreateStatic() 静态创建一个任务
xTaskCreateRestricted() 创建一个动态MPU（Memory Protection Unit），一般不使用
vTaskDelete() 删除任务
	
  修改时间：2024-05-16
	1.增加压电ADC硬件定时器中断采样，打印数据
	2.增加低通滤波器滤波
	3.采样频率过高导致波形噪音大，设置定时器0.05s,主延时0.1s,10hz

  修改时间：2024-05-17
	1.经测试定时器采样时间间隔80ms波形较好（1199U，7999U）
	2.增加高通滤波器形成带通滤波器
		alpha参数控制了滤波器的响应速度，通过测试调节参数
		较小的alpha值会使滤波器响应更慢，更容易过滤掉高频噪声；
		较大的alpha值则会使滤波器响应更快，但可能会让更多的高频噪声通过
	3.上面高通滤波器基于平均值的方法来计算滤波器的输出，不方便，废弃
	4.重构高通滤波器
		基于RC网络的方法来计算滤波器的输出。
		它直接计算了滤波器的alpha值，来控制滤波器的响应速度
		这个值基于输入样本的时间间隔dt和滤波器的截止频率cutoffFreq。

  修改时间：2024-05-20
	1.滑动窗口导致心跳不明显，故去除
	2.高通滤波没有把呼吸心率分离，需要解决
	3.增加心跳检测函数，通过心跳上升沿60检测心跳波峰
	4.为了计算心率需要时间：
		通过采样数据数量推测时间
		通过主循环++计算延时时间
		通过定时器计算时间
		通过系统时钟时间计算时间差 
	5.为什么加了心跳检测函数，打印变慢了
		打印速度是慢了，但测试确实是13HZ
	6.task1从来都没有运行
		NVIC优先级和RTOS中断冲突，中断优先级太高抢了RTOS，优先级不兼容
		注释掉ADC的中断线配置就可以循环执行任务了，
		因为标志位清除函数用错了，不是因为中断冲突
	7.增加二值信号量来同步ISR中的全局变量数据访问，保证线程安全
	
  修改时间：2024-05-21
	1.解决中断优先级导致的任务抢占，使用软件中断触发ADC就没问题
		因为标志位清除函数用错了，不是因为中断冲突
	2.为什么同样采样时间，硬件采样平稳，软件有正弦波
	3.低通滤波后通过上升沿计算心率效果还行 ，v.5-21-3
	4.带通0.8-5hz下降沿监测心率，心跳幅度阈值由40变为10
	5.高通滤波后带正负，心跳波峰检测参数不能使用uint,改用float
	
  修改时间：2024-05-22
	1.需要判断时间计算对不对，以及将最近几次心率平均
	!!2.心跳波峰之间可能夹杂了小的心跳，导致不是真实心跳间期，还是要过滤呼吸
	3.通过滑动数组取最近几次RR心率平均值
	4.如何通过波峰检测心率更可靠
	5.一个很大或很小的心率值对整体影响较大
	
  修改时间：2024-05-24
	1.波峰波谷更新数值有误，应该更新为上一个ADC值
	2.延时2000才差不多一秒,因为rtos的MCU时钟频率没改
	
  修改时间：2024-05-25
	1.腰部信号存在一个呼吸频率的波，可能是电容充放电导致的，
	2.直接把ADC值×5放大检测
	
  修改时间：2024-05-30
	1.增加区域极大值呼吸检测,使用移动平均滤波器平滑处理寻找波峰
	2.待修改：平均呼吸率。阈值过滤非呼吸波峰，FFT实现心率波峰
	
  修改时间：2024-06-11
	1.心率信号大了反而捕捉不到心率为什么
	
  修改时间：2024-06-24
	1.坐垫心率呼吸测试，波形能看到心率波形，下降沿大于50阈值判断
	  但是呼吸检测就有问题了，需要调整，并且体动需要过滤

  修改时间：待修改
	2.因为只能判断相邻波峰，如果中间掺杂了小心跳会导致心率变小
	3.考虑使用二阶或更高阶的滤波器来获得更陡峭的滚降特性
	4.增加卡尔曼进行自适应算法滤波，能够学习和适应用户特定的心跳模式。
	7.提高ADC分辨率获得更清晰信号
	8.利用MATLAB或Python等信号处理工具箱，
	  利用内置的滤波器设计工具和算法，快速测试和验证不同的滤波方案
	10.使用中位数过滤器或其他鲁棒统计方法来进一步平滑数据，并排除异常心跳间期（如由运动引起）。
	11.对于明显偏离平均水平的心率读数，可以选择丢弃它们或者用前后心率读数的加权平均来代替。
	12.利用历史数据进行预测
	13.如果可能，根据当前信号质量动态调整采样率，以在允许范围内最大化分辨率
	14.引入一个动态阈值而不是固定阈值（HEART_RATE_THRESHOLD），
		该动态阈值可以根据最近几个心跳周期内波形振幅的统计特征来调整
	15.目前的峰值检测方法是基于单个样本点之间的变化，这可能会受到噪声的影响。
		可以尝试使用更复杂的方法，比如基于窗口内局部最大值搜索。
		
		
		
	修改时间：2024/7/17
		1.模板增加startup和core以及标准固件库，不需要从Keil导入库函数包，keil只需芯片包即可运行
