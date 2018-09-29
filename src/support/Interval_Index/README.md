InterVal Index帮助文档
=======================
-----------------------
汤 琦 <tangqi@iie.ac.cn>
郑 超 <zhengchao@iie.ac.cn>  

> 注意: InterVal Index仍然在维护中，如有疑问，请及时联系<tangqi@iie.ac.cn>或<zhengchao@iie.ac.cn>



区间索引
---------------------------
&emsp;&emsp;区间索引（InterVal Index，以后简称IVI）是一个用于处理区间操作的一个C语言工具库。我们要求所有的区间都是闭区间，并且在IVI中存储的区间之间没有重叠（overlap）。IVI支持区间端点回绕(rollback)。

### <u>我们能做什么？</u> ###
IVI能够支持的区间操作有：  
&emsp;1. insert  
&emsp;2. remove  
&emsp;2. query  
&emsp;3. traverse  
&emsp;4. count  
&emsp;5. relation  
具体的更加详细的功能下面将详细说明。
### <u>如何使用？</u> ###
IVI库文件的组织形式如下图所示：

         |-include/
         |        |-interval_index.h        //头文件
         |
         |-----src/
         |        |-interval_index.c        //源文件
         |        |-Makefile               
	     |
         |-----lib/
         |        |-lib_interval_index.a    //静态链接库
         |        |-lib_interval_index.so   //动态链接库
         |
         |--sample/
         |        |-sample.c                //例子
         |        |-Makefile
         |
         |--help文档

  你的工作是：
（1）根据需求（参考章节“区间的结构”）决定是否修改头文件```interval_index.h```;
（2）如有改动头文件，则需要重新编译IVI：
```
cd ./src
make
```
（3）在自己的源代码中包含头文件```#include "interval_index.h" ```，并根据自己的需求链接动态链接库或静态链接库。



区间的结构
---------------------------
&emsp;&emsp;我们对于区间的结构体定义如下：
```C
typedef struct __IVI_seg_t{
    OFFSET_TYPE left;
    OFFSET_TYPE right;
    void * data;
}IVI_seg_t;
```
因此创建一个区间时，你需要提供的参数有区间左端点left、区间右端点right和区间所关联的用户数据 * data。

&emsp;&emsp;在IVI中，所有的区间都是闭区间，区间端点的类型是```OFFSET_TYPE```类型，它默认是```unsigned int```。如果你需要使用```unsigned long```，那么你需要修改```interval_index.h```文件，在文件开头添加```#define SIZE_8```，如下所示:

interval_index.h:
```C
#ifndef _INTERVAL_INDEX_H_
#define _INTERVAL_INDEX_H_

#ifdef __cplusplus
extern "C"{
#endif

#define SIZE_8  //当你需要使用unsigned long时，在此处添加宏定义

#ifdef  SIZE_8
typedef unsigned long OFFSET_TYPE;
typedef signed long S_OFFSET_TYPE;
#else
typedef unsigned int OFFSET_TYPE;
typedef signed int S_OFFSET_TYPE;
#endif
```

**注意**：由于IVI支持端点回绕，因此**不要**直接比较两个区间端点的大小！使用我们提供的几个内联函数来判断端点之间的关系：
```C
//如果off1 < off2，返回1， 否则返回0
static inline int before(OFFSET_TYPE off1, OFFSET_TYPE off2);

//如果off2 > off1，返回1， 否则返回0
#define after(off2, off1) before(off1, off2)

//如果prev在next之前，并且是连续的数，返回1， 否则返回0
static inline int continuous(OFFSET_TYPE prev, OFFSET_TYPE next);
```

区间的操作
---------------------------
### <u>（1）创建一个IVI</u> ###
**函数原型**：  
```C
IVI_t * IVI_create(void);
```
**参数**：void  
**返回值**：一个IVI的的句柄；  
**说明**：所有的区间操作都是在IVI的基础上进行的，因此必须首先要调用此函数创建一个IVI句柄。    

### <u>（2）销毁一个IVI</u> ###
**函数原型**：
```C
void IVI_destory(IVI_t * handler, IVI_callback_t cb, void * usr_para);
```
**参数**： handler：通过IVI创建函数返回的IVI的句柄；  
&emsp;&emsp;&emsp; cb：回调函数，用于用户释放seg中的void * data的内存；  
&emsp;&emsp;&emsp; usr_para：回调函数的用户参数；  
**返回值**：void  
**说明**：销毁IVI，释放IVI中所有的区间所占内存，**注意**我们需要用户传入一个回调函数，用户释放区间的data部分的内存，最后释放IVI所用的内存。    


### <u>（3）为区间分配内存并创建</u> ###
**函数原型**：
```C
IVI_seg_t * IVI_seg_malloc(OFFSET_TYPE left, OFFSET_TYPE right, void * data);
```
**参数**： left： 区间的左端点值；  
&emsp;&emsp;&emsp; right：区间的右端点值；  
&emsp;&emsp;&emsp; data：区间关联的用户数据；  
**返回值**：所创建的区间的结构体指针；  
**说明**：**注意**，不要调用free()函数释放此函数返回的区间结构体，而要使用我们提供的IVI_seg_free()函数来释放内存。  


### <u>（4）释放区间内存</u> ###
**函数原型**：
```C
void IVI_seg_free(IVI_seg_t * seg, IVI_callback_t cb, void * usr_para);
```
**参数**：seg：通过IVI_seg_malloc函数得到的区间结构体的指针；  
&emsp;&emsp;&emsp;  cb：用于释放seg中的用户数据*data的回调函数；  
&emsp;&emsp;&emsp;  usr_para：回调函数的用户参数；  
**返回值**：void；  
**说明**：**注意**，seg必须通过此函数来释放，而不能调用free()函数释放。  


### <u>（5）插入区间</u> ###
**函数原型**：
```C
int IVI_insert(IVI_t * handler, IVI_seg_t * seg);
```
**参数**： handler：待插入区间的IVI的句柄；  
&emsp;&emsp;&emsp; seg：待插入的区间，由IVI_seg_malloc创建得到；  
**返回值**：如果插入成功，返回0；如果失败，返回-1；  
**说明**：如果待插入的区间和IVI中已存在的区间之间存在重叠（overlap），则插入失败，函数返回-1。  


### <u>（6）移除区间</u> ###
**函数原型**：
```C
int IVI_remove(IVI_t * handler, IVI_seg_t * seg);
```
**参数**： handler：待移除的区间所在的IVI的句柄；  
&emsp;&emsp;&emsp; seg：待移除的区间，由IVI_seg_malloc创建得到；  
**返回值**：如果移除成功，返回0；如果失败，返回-1。  
**说明**：移除区间只是把seg从IVI中删掉，但是seg的内存并没有被释放掉，IVI销毁的时候也不会释放被移除的seg，因此调用被移除的区间不再使用时，要有用户手动释放掉。释放需要调用函数IVI_seg_free()。  

### <u>（7）查询</u> ###
**函数原型**：
```C
int IVI_query(IVI_t * handler, OFFSET_TYPE left, OFFSET_TYPE right, IVI_seg_t *** segs);
```
**参数**： handler：IVI句柄；  
&emsp;&emsp;&emsp; left：查询的左边界；  
&emsp;&emsp;&emsp; right：查询的右边界；  
&emsp;&emsp;&emsp; segs：用于返回查询到的区间指针的数组；  
**返回值**：返回IVI中与用户查询的区间发生重叠关系的区间个数，如果失败（handler为空或者right值比left小），返回-1；  
**说明**：此函数用于查询给定区间与IVI中已有区间发生重叠的区间个数，并且把发生重叠的区间的指针存储在\*segs数组中。\*segs不需要由用户分配内存，但是需要由用户释放，用户不能释放\*seg数组中的元素。此函数使用的方法如下：
```C
IVI_seg_t ** overlap_segs; //声明指针数组，不用分配内存
int overlap_cnt, i;
if((overlap_cnt = IVI_query(handler, 100, 900, &overlap_segs)) < 0)
{
	printf("query error!\n");
	exit(-1);
}
for(i = 0; i < overlap_cnt; i++)
{
	printf("interval: [%u, %u] overlaps\n", overlap_segs[i]->left, overlap_segs[i]->right);
}

free(overlap_segs);   //释放指针数组
```

### <u>（8）查询连续</u> ###
**函数原型**：
```C
int IVI_query_continuous(IVI_t * handler, OFFSET_TYPE left, OFFSET_TYPE right, IVI_seg_t *** segs);
```
**参数**： handler：IVI句柄；  
&emsp;&emsp;&emsp; left：查询的左边界；  
&emsp;&emsp;&emsp; right：查询的右边界；  
&emsp;&emsp;&emsp; segs：用于返回查询到的区间指针的数组；  
**返回值**：返回IVI中与用户查询的区间发生重叠关系最左连续区间个数，如果失败（handler为空或者right值比left小），返回-1；  
**说明**：此函数用于查询给定区间与IVI中已有区间发生重叠的最左连续区间个数，并且把发生重叠的最左连续区间的指针存储在\*segs数组中。  
&emsp;&emsp;&emsp;最左连续区间的含义是在所有的区间中，从左边第一个区间起，直到一个区间与前一个区间端点不连续为止（不包括此区间）的一系列连续区间。例如区间[1, 3]， [4,  6]，[8, 10]，[11, 16]，[18, 20]的最左连续区间是[1, 3]，[4, 6]。  
&emsp;&emsp;&emsp;同样，\*segs不需要由用户分配内存，但是必须由用户释放，用户不能释放\*seg数组中的元素。此函数使用的方法如下：
```C
IVI_seg_t ** overlap_segs; //声明指针数组，不用分配内存
int overlap_cnt, i;
if((overlap_cnt = IVI_query_continuous(handler, 100, 900, &overlap_segs)) < 0)
{
	printf("query error!\n");
	exit(-1);
}
for(i = 0; i < overlap_cnt; i++)
{
	printf("interval: [%u, %u] overlaps\n", overlap_segs[i]->left, overlap_segs[i]->right);
}

free(overlap_segs);   //释放指针数组
```

### <u>（9）遍历IVI</u> ###
**函数原型**：
```C
void IVI_traverse(IVI_t * handler, IVI_callback_t cb, void * usr_para);
```
**参数**： handler：待遍历的IVI的句柄；  
&emsp;&emsp;&emsp; cb：用户的想要执行的回调函数；  
&emsp;&emsp;&emsp; usr_para：回调函数所需的用户参数；  
**返回值**：void  
**说明**：此函数遍历IVI中的所有区间，对于遍历得到的每个区间执行一次回调函数cb。回调函数的原型如下：
```C
typedef void IVI_callback_t(IVI_seg_t * seg, void * usr_para);
```
**注意**，IVI_traverse()函数是可重入的函数。


### <u>（10）区间位置关系</u> ###
**函数原型**：
```C
Relation_t IVI_relative_position(IVI_seg_t * seg1, IVI_seg_t * seg2);
```
**参数**： seg1：位置关系中的主语；  
&emsp;&emsp;&emsp;  seg2：位置关系中的宾语；  
**返回值**：seg1相对与seg2的位置关系；  
**说明**：这个函数是判断seg1相当于seg2的位置关系，相对位置关系包括如下6种：  
```C
typedef enum __Relation_t{
    LEFT_NO_OVERLAP = 1,                //  |___A___|
                                        //             |___B___|

    LEFT_OVERLAP,                       //  |___A___|
                                        //        |___B___|

    CONTAINED,                          //   |___A___|
                                        // |_____B_____|

    CONTAIN,                            // |_____A_____|
                                        //   |___B___|

    RIGHT_OVERLAP,                      //        |___A___|
                                        //  |___B___|

    RIGHT_NO_OVERLAP,                   //             |___A___|
                                        //  |___B___|

    ERROR
}Relation_t;
```

### <u>（11）前一个连续区间</u> ###
**函数原型**：
```C
IVI_seg * IVI_prev_continuous_seg(IVI_seg_t * seg);
```
**参数**： seg：给定的区间；  
**返回值**：返回seg区间的前一个连续的区间，没有则返回NULL；  
**说明**：连续的意思是seg的前面一个区间的右端点和seg的左端点是连续的。  


### <u>（12）后一个连续区间</u> ###
**函数原型**：
```C
IVI_seg * IVI_next_continuous_seg(IVI_seg_t * seg);
```
**参数**： seg：给定的区间；  
**返回值**：返回seg区间的后一个连续的区间，没有则返回NULL；  
**说明**：连续的意思是seg的右端点和seg的后面一个区间的左端点是连续的。  

### <u>（13）前一个区间</u> ###
**函数原型**：
```C
IVI_seg * IVI_prev_seg(IVI_seg_t * seg);
```
**参数**： seg：给定的区间；  
**返回值**：返回seg区间的前一个区间，没有则返回NULL；  
**说明**：seg必须已经插入到区间索引中。  

### <u>（14）后一个区间</u> ###
**函数原型**：
```C
IVI_seg * IVI_next_seg(IVI_seg_t * seg);
```
**参数**： seg：给定的区间；  
**返回值**：返回seg区间的后一个区间，没有则返回NULL；  
**说明**：seg必须已经插入到区间索引中。  

### <u>（15）第一个区间</u> ###
**函数原型**：
```C
IVI_seg * IVI_first_seg(IVI_t * handler);
```
**参数**： handler: 给定的IVI；  
**返回值**：返回handler中的第一个区间，没有则返回NULL；  

### <u>（16）最后一个区间</u> ###
**函数原型**：
```C
IVI_seg * IVI_last_seg(IVI_t * handler);
```
**参数**： handler: 给定的IVI；  
**返回值**：返回handler中的最后一个区间，没有则返回NULL；  

### <u>（17）区间个数统计</u> ###
**函数原型**：
```C
int IVI_seg_cnt(IVI_t * handler);
```
**参数**： handler：待统计的IVI的句柄；  
**返回值**：IVI中区间的个数；  


### <u>（18）区间长度统计</u> ###
**函数原型**：
```C
OFFSET_TYPE IVI_seg_length(IVI_t * handler);
```
**参数**： handler：待统计的IVI的句柄；  
**返回值**：IVI中所有区间的总长度；  

### <u>（19）区间索引内存统计</u> ###
**函数原型**：
```C
unsigned long long IVI_mem_occupy(IVI_t * handler);
```
**参数**： handler：待统计的IVI的句柄；  
**返回值**：IVI占用的内存总数，只统计插入到区间中的Seg；  
**说明**：不包括用户数据的内存  
