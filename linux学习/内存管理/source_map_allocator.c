/*******************************************************************************
 * Created Time:    2017-05-10 09:42:08
 * Modified Time:   2017-05-10 20:04:54
 * Function：该程序模拟资源映射图分配内存使用空闲链表保存未分配的内存模块，采用最先匹配算法
 *       通过该程序可以看出资源映射图分配内存时会产生大量的内存碎片，内存浪费率很高！
 * Author：JIN dizhao
 * *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAX_MEM_SIZE 100   //用于模拟的内存池大小
#define MAX_SIZE_PE_BLOCK 50 //内存块最大size

/*以下为使用到的数据结构*/
//定义链表节点
typedef struct node
{
    unsigned int *base;  //空闲内存块基地址
    unsigned int size;  //空闲内存块大小
    struct node *next;  //下一个节点
}data_node;
//定义空闲链表表头数据结构
typedef struct free_mem
{
    int free_count; //空闲内存块数量
    int free_total_size;    //空闲内存总数量
    data_node *free_list;  //指向空闲链表头部
}free_mem_list;
//定义使用链表表头数据结构
typedef struct used_mem
{
    data_node *used_list;
}allocated_mem_list;

//用于模拟的内存池地址,unsigned int 型占用32个字节，用unsigned int来作为内存池的最小单元
unsigned int *memory;   

/*以下为空闲链表操作函数*/
//内存分配最先匹配算法
unsigned int *alloc_mem_firstmatch(free_mem_list *free_memory_list,allocated_mem_list *allocated_memory_list, int size);
//内存分配最佳匹配算法
unsigned int *alloc_mem_besttmatch(free_mem_list *free_memory_list, int size);
//内存分配最差匹配算法
unsigned int *alloc_mem_worsttmatch(free_mem_list *free_memory_list, int size);
//内存释放函数
int free_mem(free_mem_list *free_memory_list, allocated_mem_list *allocated_memory_list,unsigned int *mem_to_be_freed, int size);
//空闲链表初始化函数
void free_mem_list_init(free_mem_list * free_memory_list);



int main(int argc, char *argv[])
{
    free_mem_list *free_memory_list = NULL;
    allocated_mem_list *allocated_memory_list = NULL;
    data_node *each;
    int input_size; 
    int free_size; //随机释放的内存块大小

    //获取随机种子，之后产生随机数时会用到
    srand(time(NULL));

    //为空闲链表表头开辟内存
    free_memory_list = (free_mem_list*)malloc(sizeof(free_mem_list));
    if(free_memory_list == NULL)
        return -1;
    //为使用链表表头开辟内存
    allocated_memory_list = (allocated_mem_list*)malloc(sizeof(allocated_mem_list));
    if(allocated_memory_list == NULL)
        return -1;
    allocated_memory_list->used_list = NULL;
    //分配由于模拟的内存池，malloc不能保证在物理空间内连续，但这不影响后续模拟
    memory = (unsigned int *)malloc(sizeof(unsigned int)*MAX_MEM_SIZE);
    if(memory == NULL)
        return -1;
    
    //初始化空闲链表
    free_mem_list_init(free_memory_list);

    //模拟内存分配和释放过程
    printf("输入需要分配的n个内存块大小(大小<=50)，以空格分割开，单位为unsigned int\n按enter键执行分配，在分配过程中会随机释放内存，按Ctr+d 结束测试\n");
    while(scanf("%d",&input_size) != EOF) 
    {
        unsigned int *p;
        p = alloc_mem_firstmatch(free_memory_list, allocated_memory_list,input_size);
        if(p == NULL)
        {
            printf("分配失败！\n");
            //分配失败时打印空闲链表统计信息
            printf("剩余内存块数：%d, 剩余内存大小：%d\n, 浪费率%f\n",free_memory_list->free_count, free_memory_list->free_total_size, (float)free_memory_list->free_total_size/MAX_MEM_SIZE);
        }
        else
           printf("allocate: start %p，end %p, size %d\n", p,p+input_size, input_size);

        //随机释放部分内存
        each = allocated_memory_list->used_list;
        while(each != NULL)
        {
            //产生-5到5的随机数，如果大于等于3，则可释放该部分内存，然后根据
            //内存块size的大小产生1--size的随机数，表示释放的内存大小。
            if((rand()%11)-5 >= 3)
            {
                unsigned int *free_mem_addr;
                free_size = rand()%(each->size) + 1;
                free_mem_addr = each->base;
                //释放内存
                free_mem(free_memory_list, allocated_memory_list, free_mem_addr, free_size);
                printf("free：start %p, end %p, size %d\n", free_mem_addr, free_mem_addr + free_size, free_size);
            }

            each = each->next;
        }
    }
    
    return 0;
}

void 
free_mem_list_init(free_mem_list *free_memory_list)
{
    /*该函数初始化空闲链表
     * 输入：空闲链表free_memory_list
     */
    data_node *free_node;
    //为空闲链表的第一个节点分配内存
    free_node = (data_node *)malloc(sizeof(data_node));
    if(free_node == NULL)
        return;
    //初始化第一个节点
    free_node->base = memory;
    free_node->size = MAX_MEM_SIZE;
    //初始化空闲链表
    free_memory_list->free_count = 1;
    free_memory_list->free_total_size = MAX_MEM_SIZE;
    free_memory_list->free_list = free_node;

    return;
}

unsigned int 
*alloc_mem_firstmatch(free_mem_list *free_memory_list,allocated_mem_list *allocated_memory_list, int size)
{
    /* 功能：该函数采用最先匹配算法分配内存
     * 输入：空闲链表 free_memory_list,使用链表allocated_memory_list, 需分配的内存大小size
     * 输出：分配的内存指针
     */

    data_node *node, *prev, *new;
    unsigned int *start;

    if(size == 0)
        return NULL;
    if(free_memory_list->free_total_size < size)
        return NULL;
    if(free_memory_list->free_count == 0)
        return NULL;

    //在空闲链表中寻找最先满足size要求的内存块
    prev = node = free_memory_list->free_list;
    while(node != NULL)
    {
        if(node->size >= size)//找到满足要求的内存块
        {
            if(node->size == size)  //恰好满足size
            {
                //空闲链表中删除该内存块
                if(prev == node)    //要删的内存块在表头
                    free_memory_list->free_list = node->next;
                else
                    prev->next = node->next;
                free_memory_list->free_count--;
                new = node;
                start = node->base;
            }
            else    //node->size >size时将剩余部分加入空闲链表
            {
                start = node->base;   
                node->base += size;
                node->size -= size;

                //为使用链表分配一个新节点
                new = (data_node *)malloc(sizeof(data_node));
                if(new == NULL)
                   return NULL;
                new->base = start;
                new->size = size;

            }

            //更新空闲链表中总内存大小
            free_memory_list->free_total_size -= size;

           //将分配的内存块移入使用链表表头
            new->next = allocated_memory_list->used_list;
            allocated_memory_list->used_list = new;
            
            return start;
        }
        else
        {
            prev = node;
            node = node->next;
        }
    }

    return NULL;
}

int 
free_mem(free_mem_list *free_memory_list,allocated_mem_list *allocated_memory_list, unsigned int *mem_to_be_freed, int size)
{
    /* 功能：该函数释放特定size的内存，释放时可以只释放部分内存
     * 输入：空闲链表free_memory_list,使用链表allocated_memory_list，
     *       需要释放的内存指针mem_to_be_freed,
     *       需要释放的内存大小size。此处指明了空闲链表，是因为仅根据内存块指针找不
     *       到其所在的
     *       空闲链表，如果需要这样，在内存块的头部存放一个指针，指向所在的空闲链表即可
     * 输出：返回-1表示释放失败，0表时释放成功
     */
    data_node *node, *prev, *new;


    if(size == 0)
        return -1;
    //在使用链表中找到需要释放的内存块,插入空闲链表
    prev = node = allocated_memory_list->used_list;
    while(node != NULL)
    {
        if(node->base == mem_to_be_freed)
        {
            if(size > node->size)   //size大于实际使用的内存块大小
                return -1;
            else if(size == node->size) //恰好等于实际内存块大小
            {
                if(prev == node)    //内存块在表头
                    allocated_memory_list->used_list = node->next;
                else
                    prev->next = node->next;
                new = node;//如果这样则不必为空闲链表分配新的节点，直接使用node
            }
            else    //只释放一部分,从低地址开始
            {
                node->base += size;
                node->size -= size;

                //为空闲链表分配新的节点
                new = (data_node*)malloc(sizeof(data_node));
                if(new == NULL)
                   return -1;
                new->base = mem_to_be_freed;
                new->size = size;
            }
           //将释放的内存块插入空闲链表头
           new->next = free_memory_list->free_list;
           free_memory_list->free_list = new;
           //更新空闲链表统计数据
           free_memory_list->free_count++;
           free_memory_list->free_total_size += size;
           return 0;
        }
        else
        {
            prev = node;
            node = node->next;
        }
    }

       return -1;
}




