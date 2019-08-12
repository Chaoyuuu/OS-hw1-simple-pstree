#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/pid.h>
#include <linux/list.h>
#include <linux/sched.h>
#define NETLINK_USER 31

struct sock *nl_sk = NULL;
char str1[20000];
int max_level;

void print_child(struct task_struct * task_tmp, int level)
{

    struct task_struct * task_child;
    struct list_head * list = NULL;
    level++;
    if(list_empty(&task_tmp->children)) {
        printk(KERN_INFO " nothinggggggggggggg\n");
        return;
    }
    list_for_each(list, &task_tmp->children) {
        task_child = list_entry(list, struct task_struct, sibling);
        //printk(KERN_INFO "child's pid = %s(%d)\n", task_child->comm, task_child->pid);

        char tmp[100];
        int i=0;
        for(i=0; i<level; i++) {
            strcat(str1, "   ");
        }
        sprintf(tmp, "%s(%d)\n", task_child->comm, task_child->pid);
        strcat(str1,tmp);
        print_child(task_child, level);
    }
}

void print_sibling(struct task_struct * task_tmp, int num)
{
    struct task_struct * task_sibling;
    struct list_head * list = NULL;

    list_for_each(list, &task_tmp->parent->children) {
        task_sibling = list_entry(list, struct task_struct, sibling);
        if(task_sibling->pid == num) {
            strcat(str1, "");
            //printk(KERN_INFO " i am the question\n");
        } else {
            //printk(KERN_INFO "in sibling, pid = %d\n",task_sibling->pid);
            char tmp[100];
            sprintf(tmp, "%s(%d)\n", task_sibling->comm, task_sibling->pid);
            strcat(str1, tmp);
        }
    }
    return;
}
void print_parent(struct task_struct * task_tmp, int level)
{
    //printk(KERN_INFO " in function\n");

    struct task_struct * task_parent;
    struct pid * pid_struct_1;
    struct task_struct * task_1;
    struct pid * pid_struct_2;
    struct task_struct * task_2;
    char tmp[100];

    level++;
    pid_struct_1 = find_get_pid(1);
    task_1 = pid_task(pid_struct_1, PIDTYPE_PID);
    pid_struct_2 = find_get_pid(2);
    task_2 = pid_task(pid_struct_2, PIDTYPE_PID);

    if(task_tmp->parent == task_1 || task_tmp->parent == task_2) {
        //printk(KERN_INFO " the end\n");
        max_level = level;
        sprintf(tmp, "%s(%d)\n", task_tmp->parent->comm, task_tmp->parent->pid);
        strcat(str1, tmp);
        return;
    } else {

        task_parent = task_tmp->parent;
        print_parent(task_parent, level);
        int i=0;
        for(i=0; i<max_level-level; i++) {
            strcat(str1, "   ");
        }
        sprintf(tmp, "%s(%d)\n", task_parent->comm, task_parent->pid);
        strcat(str1, tmp);
        //printk(KERN_INFO " my parent = %s(%d), max = %d", task_parent->comm, task_parent->pid, max_level-level);
    }
}

static void hello_nl_recv_msg(struct sk_buff *skb)
{

    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int res;
    struct pid * pid_struct;
    struct task_struct * task;
    int num;

    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);
    nlh = (struct nlmsghdr *)skb->data;
    printk(KERN_INFO "Netlink received msg payload:%s\n", (char *)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */
    char * data_tmp= nlmsg_data(nlh);


    if(kstrtoint(data_tmp+strlen("00"), 10, &num) != 0) {
        printk(KERN_INFO "the wrong input num\n");
    }
    //printk(KERN_INFO " option=%d, num=%d", option, num);
    printk(KERN_INFO " the num = %d\n", num);
    memset(str1, '\0', strlen(str1));

    if(find_get_pid(num) == NULL && num != 0) {
        printk(KERN_INFO "no pidddd\n");
        strcat(str1, "no pid");
    } else {

        if(strncmp(data_tmp+strlen("0"), "c", 1) == 0) {
            //do children
            printk(KERN_INFO "in c\n");

            if(num == 0) {
                num = 1;
            }
            pid_struct = find_get_pid(num);
            task = pid_task(pid_struct, PIDTYPE_PID);
            sprintf(str1, "%s(%d)\n", task->comm, task->pid);
            print_child(task, 1);

        } else if(strncmp(data_tmp+strlen("0"), "s", 1) == 0) {
            //do sibling
            printk(KERN_INFO " in s\n");
            if(num == 0) {
                num = current->pid;
            }

            pid_struct = find_get_pid(num);
            task = pid_task(pid_struct, PIDTYPE_PID);
            print_sibling(task, num);


        } else if(strncmp(data_tmp+strlen("0"), "p", 1) == 0) {
            //do parent
            printk(KERN_INFO " in p\n");
            //printk(KERN_INFO "current = %s(%d)\n", current->comm, current->pid);
            if(num == 0) {
                num = current->pid;
            }

            pid_struct = find_get_pid(num);
            task = pid_task(pid_struct, PIDTYPE_PID);
            print_parent(task, 1);

            int i=0;
            for(i=0; i<max_level-1; i++) {
                strcat(str1, "   ");
            }

            char tmp[100];
            sprintf(tmp, "%s(%d) \n", task->comm, task->pid);
            strcat(str1, tmp);

        } else {
            strcat(str1, "no pid");
        }

    }

    skb_out = nlmsg_new(strlen(str1), 0);
    if (!skb_out) {
        printk(KERN_ERR "Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, strlen(str1), 0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    strncpy(nlmsg_data(nlh), str1, strlen(str1));
    res = nlmsg_unicast(nl_sk, skb_out, pid);

    printk(KERN_INFO "str1 = %s\n", nlmsg_data(nlh));

    if (res < 0)
        printk(KERN_INFO "Error while sending bak to user\n");
}

static int __init hello_init(void)
{

    printk("Entering: %s\n", __FUNCTION__);

    struct netlink_kernel_cfg cfg = {
        .input = hello_nl_recv_msg,
    };

    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        return -10;
    }

    return 0;
}

static void __exit hello_exit(void)
{

    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
