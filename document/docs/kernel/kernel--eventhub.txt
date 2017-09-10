EVENTHUB
eventhub.c

int main(int argc, char *argv[])
     eventd_serv_init_signal();
     eventd_serv_init();
          eventd_handler_key = eventd_provider_key = getpid();
          eventd_handler_ID = eventd_handler_msg_queue_create(getpid());
          eventd_provider_ID = eventd_provider_msg_queue_create(getpid());
          provider_ID = provider_msg_queue_create(getpid());
          //没有定义handler_ID　和　handler_key
     createNodeList();
     create_info_head();
     create_linstener_head();
     create_threads();
          pthread_mutex_init and pthread_cond_init;
          err = pthread_create(&stat_ntid, NULL, stat_thread_fun, NULL);//每0.5s显示info
     register_thread_fun(NULL);　//while(1)处理
          struct msg_register_buffer register_data.msg_type = HANDLER_REGISTER_TYPE;
          err = Read_Handler_Register_Event(&register_data);
          if(0 == err)
               register_handler(&(register_data.event));//or unregister_handler()
               register_data.msg_type = HANDLER_REGISTER_RETURN;
               msg_id = msg_queue_create(register_data.event.pid);
               Send_Provider_Register_Return(msg_id, &register_data);
          else     rec_data_thread_fun(NULL);//非注册注销消息
               provider_data.msg_type = DATA_TYPE;     err = Read_Data_Event_From_Provider(&provider_data);
               if(0 == err)
                    if(-1 == event_handler_registered(provider_data))　return;
                    new_list = malloc(sizeof(struct data_list));     //然后用来保存读取到的消息内容
                    addNode(new_list);　//添加消息内容到data_list_head
               else if(NULL == data_list_head->pNext)     usleep(5000);
          send_data_thread_fun(NULL);
               current_list = data_list_head->pNext;
               //如果current_list->event_name不包含在event_name_list[]数组中，deleteNode(current_list);返回

     if go here, then destroy all resources;
void * register_thread_fun(void *arg)
     register_data.msg_type = HANDLER_REGISTER_TYPE;
     err = Read_Handler_Register_Event(&register_data);//读取eventd_handler_ID消息队列handler消息
     status = register_handler(&(register_data.event));　or
     status = unregister_handler(&(register_data.event));//注册或者注销一个handler
     msg_id = msg_queue_create(register_data.event.pid);//取得消息的对应线程的消息队列．
     Send_Provider_Register_Return(msg_id, &register_data);//发送response到对端消息队列.
     send_data_thread_fun(NULL);

libevent.c
/*本进程需要处理handler,才会调用event_register_handler*/
/*增加注册handler，建立线程读取进程消息队列中别的进程发送的event，调用对应handler处理*/
/*并通知eventhub本进程handler注册情况并接收eventhub的response，多个进程都会用到eventhub的消息队列*/
int event_register_handler(const char *event, const int priority, void (* handler)(char* event, void *arg))
     strncpy(func_array[handler_num].name, event, strlen(event)+1);
     func_array[handler_num].func_handler_list.func_handler = handler;
     func_array[handler_num].func_handler_list.next = NULL;
     func_array[handler_num].priority = priority;
     res = handler_init(); //保证当前进程和eventhub进程各自创建handler相关的消息队列．
          get_eventd_pid(); //eventd_pid赋值为eventhub进程
          eventd_handler_key = eventd_pid; eventd_handler_msg_queue_create(eventd_handler_key);//
          handler_msg_queue_create();//创建当前pid对应的消息队列
     if(0 == handler_ntid)　res = pthread_create(&handler_ntid, NULL, handler_thread_fun, NULL);//可能多个进程都有定义
     register_msg.msg_type = HANDLER_REGISTER_TYPE;  strncpy(register_msg.event.name, event, strlen(event)+1);
     register_msg.event.pid = getpid();     register_msg.event.priority = priority;
     register_msg.event.handler_key = getpid();   register_msg.event.event_type = EVENT_REGISTER;
     Send_Handler_Register_Event(&register_msg);
     register_msg.msg_type = HANDLER_REGISTER_RETURN;
     Read_Handler_Register_Return(&register_msg);
     handler_num++;
void *handler_thread_fun(void* arg)　//while(1)循环处理,
     status = event_get_data(buf, MAX_EVENT_DATA-1, &handler_data_msg);//从当前pid对应的消息队列中取DATA_TYPE类型的消息
          handler_msg_id = msgget(key, IPC_EXCL | IPC_CREAT | 0660);
          Read_Data_Event_From_Eventd(handler_msg_id, handler_data_msg);
          memcpy(buf, handler_data_msg->event.buf, size);
     if(0 == strcmp(handler_data_msg.event.name, func_array[i].name))//根据eventname找到func_handler.
          func_array[i].func_handler_list.func_handler(handler_data_msg.event.name, (void *)buf);
          //包块list中的回调函数的处理．
          if(1 == handler_data_msg.event.need_wait)//发送response到provider消息队列
               msg_id = msg_queue_create(key); //获取发送消息的Provider的消息队列id
               memcpy(handler_data_msg.event.buf, buf, MAX_EVENT_DATA-1);
               Send_Return_To_Provider(msg_id, &handler_data_msg);

event_unregister_handler和event_register_handler中，handler_num好像是bug.

int event_send(const char *event, char *buf, int size)
     provider_init();//保证当前进程和eventhub进程各自创建provider相关的消息队列,进程发送消息然后response被发送到此队列．
          get_eventd_pid();
          eventd_provider_key = eventd_pid;
          /*eventd_provider_key = provider_key = eventd_pid, 所有进程都使用eventhub进程的消息队列*/
          eventd_provider_msg_queue_create(eventd_provider_key);
          provider_msg_queue_create(eventd_pid);
          pthread_mutex_init(&mut, NULL);
     provider_data_msg.msg_type = DATA_TYPE;
     strncpy(provider_data_msg.event.name, event, strlen(event)+1);
     provider_data_msg.event.pid = getpid();
     provider_data_msg.event.datasize = size;
     provider_data_msg.event.need_wait = 0;
     memcpy((char*)(provider_data_msg.event.buf), buf, size);
     gettimeofday(&provider_data_msg.event.send_tv , &tz); //get time
     Send_Data_Event_To_Eventd(&provider_data_msg)

