/****************************************************
* Created Time:    2017-05-11 14:47:27
* Modified Time:   2017-05-12 17:16:13
* Function：该文件模拟改进版2的幂次方分配器，内存块的大小为32，64，128，256和512Byte，
*           每页的大小定为4KB，每页只含有一种尺寸的内存块，主要作了如下改进：
*           每页的前4*32Bit用作位图，表示页中内存块
*           的使用情况，使用则相应位置1，空闲则相应位清零。紧接着2*32个Bit存储指向下一
*           个页的指针和指向表头的指针,这块区域把它称为保留区，保留区的大小总共为24Byte。
*           该方法对造成一定的浪费，因为剩余内存不再是2的幂次方整数倍。避免浪费的方式是
*           将保留区的内容单独存
*           放，但这需要单独开辟内存，对于实际的内存分配系统来说需要另一个分配器来完成。这
*           里对这种情况不做模拟。
* Author：JIN dizhao
* **************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_PAGE_NUM 10 //用于模拟的页数量
#define PAGE_SIZE 4096  //每页大小 Byte
#define RESERVE_SZIE  32 //每页头部保留尺寸，用于位图和指针 Byte

/***以下定义用到的数据结构***/
//定义位图，最大128位，采用4个unsigned int型数
typedef unsigned int bitmap[4];
//定义页表头部保留区数据结构
typedef struct mem_head_type
{
    bitmap memory_bitmap;   //位图
    //指向所属空闲链表的指针，所指数据类型应该为free_list_type,但该类型在后面才定义，
    //因此暂用void*来表示，使用时再强制类型转换
    void *free_list;
    struct mem_head_type *next;  //下一个页
}mem_head;
//定义各种尺寸（32，64，128Byte等）空闲链表表头数据结构
typedef struct free_array
{
    unsigned int size;  //该链表中每个内存块大小
    unsigned int free_count;    //空闲内存块数量
    mem_head *head; //指向空闲链表表头的指针
}free_list_type;

//定义总的空闲链表表头数组,依次为32，64，128，256，512Byte尺寸空闲链表表头。
typedef free_list_type  free_mem_type[5];

/***以下定义用到的宏函数***/
//根据页的基地址获取页上第一个内存块的基地址
#define Get_Base_Address(pointer) (unsigned int*)(pointer + RESERVE_SZIE/sizeof(unsigned int))
//根据位图中找到对应位所表示的内存块基地址
#define Bitmap_to_Address(Base,num,size) (Base + num * size/sizeof(unsigned int))
//由内存块基地址找到位图中相应位
#define Address_to_Bitmap(Base,Address,size) ((Address - Base)*sizeof(unsigned int)/size)
//将size转换为free_mem_type数组下标
#define ROUND(size) (size > 256 ? 4 : size > 128 ? 3 : size > 64 ? 2 : size > 32 ? 1 : 0)

/*用到的函数*/
//位图相应位置1
void 
set_bit(unsigned int *pointer,unsigned int num) 
{
    /*功能：位图中相应位置1，位图用4个元素的unsigned int型数组组成，以此在找到
     *      相应位前需先找到该位所处在数组哪一个unsigned int元素上。
     *输入：指向页的指针pointer, 位号num
     */
    int temp;
    temp = num/4;
    if(temp >=0 && temp < 4) //num在第一unsigned int
        *pointer |= (0x80000000 >> num);
    else if(temp >= 4 && temp < 8)  //num在第二个unsigned int
        *(pointer+1) |= (0x80000000 >> (num - sizeof(unsigned int)));
    else if(temp >= 8 && temp < 16) //第三个unsigned int
        *(pointer+2) |= (0x80000000 >> (num - sizeof(unsigned int)*2));
    else if(temp >=16 && temp < 20) //第四个unsigned int
        *(pointer+3) |= (0x80000000 >> (num - sizeof(unsigned int)*3));
    else
        return;

    return;
}
//位图相应位清零
void 
clear_bit(unsigned int *pointer, unsigned int num)
{
    /*功能：位图中相应位清零，寻找方法同set_bit函数
     *输入：指向页的指针pointer,位号num
     */
    int temp;
    temp = num/4;
    if(temp >=0 && temp < 4) //num在第一unsigned int
        *pointer &= (0x7FFFFFFF >> num);
    else if(temp >= 4 && temp < 8)  //num在第二个unsigned int
        *(pointer+1) &= (0x7FFFFFFF >> (num - sizeof(unsigned int)));
    else if(temp >= 8 && temp < 16) //第三个unsigned int
        *(pointer+2) &= (0x7FFFFFFF >> (num - sizeof(unsigned int)*2));
    else if(temp >=16 && temp < 20) //第四个unsigned int
        *(pointer+3) &= (0x7FFFFFFF >> (num - sizeof(unsigned int)*3));
    else
        return;

    return;
}
//空闲链表初始化
int
free_list_init(free_mem_type  free_memory)
{
    /*功能：初始化2的幂次方空闲链表，为每种尺寸的空闲链表分配合适的页，并初始化位图
     *输入：空闲内存管理单元free_memory
     *输出：初始化成功返回0，失败返回-1
     */

    unsigned int *p;
    free_list_type *list;
    mem_head *memory_head;
    int i,j,k;

    for(i = 0; i < 5; i++)
    {
        //选择要初始化的链表
        list = (free_list_type*) (free_memory + i);
        //初始化
        list->head = NULL;
        list->size = 0;
        list->free_count = 0;
        //分配页面并初始化相关数据结构,每种空闲链表分配页的数量为MAX_PAGE_NUM/5个
        for(j = 0; j < MAX_PAGE_NUM/5; j++)
        {
            //分配页
            p = (unsigned int *)malloc(PAGE_SIZE);
            if(p == NULL)
                return -1;

            /*页头保留区初始化*/
            memory_head = (mem_head *)p;
            //位图初始化为0
            for(k = 0; k < 4; k++)
                memory_head->memory_bitmap[k] = 0;
            //指针初始化
            memory_head->free_list = list;
            //插入空闲链表
            memory_head->next = list->head;
            list->head = memory_head;

            list->size = 32 * (unsigned int)pow(2,i);
            list->free_count += (PAGE_SIZE-RESERVE_SZIE)/(list->size);
        }
    }
    return 0;
}
//查找位图中第一个为0的位，并返回编号
unsigned int
find_free_bit(bitmap  memory_bitmap)
{
    /*功能：查找位图中第一个为0的位
     *输入：位图memory_bitmap
     *输出：如果查找成功则返回位编号，否则返回-1
     */
    int i,j;
    unsigned int temp;
    //由于位图是由4元素的unsigned int型数组构成，因此先找到不为0xffffffff的元素
    //即位不全为1的元素
    for( i = 0; i < 4; i++)
        if(memory_bitmap[i] != 0xffffffff)
        {
            temp = memory_bitmap[i];
            //从前向后查找为0的bit
            for(j = 0; j < sizeof(unsigned int); j++)
            {

                if((~temp) & (0x80000000 >> j))
                    return i * sizeof(unsigned int) + j;
            }
        }
    return -1;
}
//寻找空闲内存块
unsigned int *
find_free_block(free_list_type * list, unsigned int size , unsigned int flag)
{
    /*功能：找出适合size的空闲内存块
     *输入：空闲链表list, 请求内存大小size,标志flag,flag大于等于1时更新位图，为0时更新
     *输出：找到的空闲内存指针
     */
    mem_head *memory_head;
    unsigned int bit_num;
    unsigned int *free_memory_base;
   if(list->free_count == 0)
        return NULL;    //该链表没有可分配的空闲内存块
    //查找每个页，寻找空闲内存块
    memory_head = list->head;
    while(memory_head != NULL)
    {
        if((bit_num = find_free_bit(memory_head->memory_bitmap))  != -1)
        {
            if(flag >= 1)
              //更新位图,相应位置1
                set_bit((unsigned int*)memory_head, bit_num);
            free_memory_base = (unsigned int*)Get_Base_Address(memory_head);
            return Bitmap_to_Address(free_memory_base, bit_num, list->size);
        }
        memory_head = memory_head->next;
    }
       return NULL;
} 

//内存分配
unsigned int *
allocate_free_memory(free_mem_type free_memory,unsigned int size)
{
    /*功能：分配空闲内存
     *输入：空闲内存管理单元free_memory, 请求内存大小size
     *输出：分配的内存指针或NULL
     */
    unsigned int *ret_addr;
    free_list_type *free_list;
    //根据size大小找到空闲链表，ROUND宏用来找到空闲表头数组下标
    free_list = (free_list_type*)(free_memory + ROUND(size));
    //寻找空闲内存块,并更新位图
    if((ret_addr = find_free_block(free_list, size, 1)) == NULL)
        return NULL;    //没有空闲内存可分配
    //更新空闲链表中剩余空闲块数量
    free_list->free_count--;

    return ret_addr;
}
//内存释放
void
free_memory(free_list_type* free_list, unsigned int * mem_to_be_freed)
{
    /*功能：释放内存，对于实际操作物理内存的2的幂次方分配器来说，释放内存只需要内存块的
     *      指针即可，因为屏蔽后12位(物理页大小为4096Byte)后变可得到页首地址，但对于该模拟
     *      程序来说，所操作的是逻辑空间，该方法行不通，因此需要另一个参数，即该内存块所属
     *      空闲链表free_list
     *输入：内存块指针mem_to_be_freed,所属空闲链表free_list。
     */
    unsigned int bit_num;
    unsigned int *free_memory_base;
    mem_head *memory_head;

    //找到要释放的内存块原来所属的页
    memory_head = free_list->head;
    while(memory_head != NULL)
    {
        if(mem_to_be_freed > (unsigned int*)memory_head && mem_to_be_freed < ((unsigned int*)memory_head+PAGE_SIZE/sizeof(unsigned int)))
            break;
        memory_head = memory_head->next;
    }
   //找到内存分配区的开头，即略过页开头保留区后的地址。
    free_memory_base = Get_Base_Address(memory_head);
    //找到对应位图中的位
    bit_num = Address_to_Bitmap(free_memory_base, mem_to_be_freed, free_list->size);
    //相应位清零
    clear_bit((unsigned int*)memory_head, bit_num);

    //更新该链表中空闲内存块数量
    free_list->free_count++;

    return;
}

            
int
main(int argc, char *argv[]) 
{
    unsigned int *p;
    unsigned int input_size, modified_size;//input_size是模拟用户请求内存大小，modified_size是修正到2的幂次方的大小
    free_mem_type free_memory_all;  //空闲内存管理单元
    //建立链表allocated_list保存已分配的内存指针,struct alloc_mem为节点数据结构
    struct alloc_mem
    {
         unsigned int *allocated_memory;    //保存分配的内存指针
         unsigned int size; //分配的内存大小
         struct alloc_mem *next;   //下一个
    } *allocated_list=NULL, *temp, *prev;
    free_list_type *list;

    //获取随机种子，用于后面的随机释放内存
    srand(time(NULL));
    //给每种尺寸的空闲链表分配适量的模拟页，并初始化
    free_list_init(free_memory_all);
    
    /*以下程序模拟内存分配和释放过程*/
    printf("请输入要分配的内存大小(<=512Byte),以空格分开，单位Byte, 按enter执行，按Ctrl+d结束模拟\n");
    while(scanf("%d", &input_size) != EOF)
    {
        if(input_size > 512)
        {
            printf("error,请求尺寸需 <= 512Byte\n");
            continue;
        }
        p = allocate_free_memory(free_memory_all, input_size);
        if(p == NULL)
            printf("分配失败\n");
        else
        {
            modified_size = free_memory_all[ROUND(input_size)].size;
            //将已分配的内存指针保存下来，用于后续释放
            temp = (struct alloc_mem*)malloc(sizeof(struct alloc_mem));
            temp->allocated_memory = p;
            temp->size = modified_size;
            //插入链表
            temp->next = allocated_list;
            allocated_list = temp;

            printf("allocated: start%p end%p size%d\n", p, p+modified_size/4,modified_size);
        }

        //随机释放内存,产生0到10的随机数，如果大于等于8则释放内存
        prev =  temp = allocated_list;
        while(temp != NULL)
        {
            if(rand()%11 >= 8)
            {
                //先找到要释放的内存块所属空闲链表
                list = (free_list_type*) (free_memory_all + ROUND(temp->size));
                free_memory(list,temp->allocated_memory);
                printf("free:start %p  size %d\n", temp->allocated_memory, temp->size);
                //从链表中删除
                if(prev == temp)    //恰好在表头
                {
                    allocated_list = temp->next;
                }
                else
                {
                    prev->next = temp->next;
                }
                free(temp);
                break;
            }
            prev = temp;
            temp = temp->next;
        }

    }
    return 0;
}

