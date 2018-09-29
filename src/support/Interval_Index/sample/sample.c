#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"interval_index.h"

#define SEGS_NUM 10

typedef struct __user_data_t{
    int id;
    double x;
    char * y;
}user_data_t;



void traverse_print_cb(IVI_seg_t * seg, void * usr_para)
{
    user_data_t * data = (user_data_t *)seg->data;
    printf("interval %d: [%lu, %lu]\n", data->id, seg->left, seg->right);
    printf("data:{%lf, %s}\n\n", data->x, data->y);
}


void free_data_cb(IVI_seg_t * seg, void * usr_para)
{
    user_data_t * data = (user_data_t *)seg->data;
    free(data->y);
    free(seg->data);
}


void test()
{
    int i, len;
    IVI_t * ivi_handler;
    IVI_seg_t * segs[SEGS_NUM];
    OFFSET_TYPE left = 1, right = 0;
    char * str = "abcdefg";

    len = strlen(str);

    ivi_handler = IVI_create();
    for(i = 0; i < SEGS_NUM; i++)
    {
        left = left * 2;
        right = left * 2 - 1;
        user_data_t * data = (user_data_t *)malloc(sizeof(user_data_t));
        data->id = i + 1;
        data->x = i + 3.14;
        data->y = (char *)malloc(sizeof(char) * (len + 1));
        memcpy(data->y, str, len);
        (data->y)[len] = '\0';
        segs[i] = IVI_seg_malloc(left, right, data);

        if(IVI_insert(ivi_handler, segs[i]) < 0)
        {
            printf("insert [%lu, %lu] error\n", segs[i]->left, segs[i]->right);
        }
        else
        {
            printf("insert [%lu, %lu] success\n", segs[i]->left, segs[i]->right);
        }
    }

    user_data_t * tmpdata = (user_data_t *)malloc(sizeof(user_data_t));
    tmpdata->id = 20;
    tmpdata->x = 1.2;
    tmpdata->y = (char *)malloc(sizeof(char) * (len + 1));
    memcpy(tmpdata->y, str, len);
    (tmpdata->y)[len] = '\0';
    IVI_seg_t * tmpseg = IVI_seg_malloc(2, 3, tmpdata);

    if(IVI_insert(ivi_handler, tmpseg) < 0)
    {
        printf("insert [%lu, %lu] error\n", tmpseg->left, tmpseg->right);
    }
    else
    {
        printf("insert [%lu, %lu] success\n", tmpseg->left, tmpseg->right);
    }

    IVI_traverse(ivi_handler, traverse_print_cb, NULL);
    IVI_destroy(ivi_handler, free_data_cb, NULL);

}



void print_cb(IVI_seg_t * seg, void * usr_para)
{
    printf("[%lu, %lu]->", seg->left, seg->right);
}



void insert_test()
{
     IVI_t * ivi_handler;

     ivi_handler = IVI_create();
     IVI_seg_t *seg1, *seg2, *seg3, *seg4, *seg5;

     seg1 = IVI_seg_malloc(18, 20, NULL);
     seg2 = IVI_seg_malloc(5, 13, NULL);
     seg3 = IVI_seg_malloc(10, 15, NULL);
     seg4 = IVI_seg_malloc(21, 30, NULL);
     seg5 = IVI_seg_malloc(14, 16, NULL);

     if(IVI_insert(ivi_handler, seg1) < 0)
     {
         printf("insert error\n");
         IVI_seg_free(seg1, NULL, NULL);
     }
     else
     {
         printf("insert success\n");
     }

     IVI_traverse(ivi_handler, print_cb, NULL);
     printf("\n");
     if(IVI_insert(ivi_handler, seg2) < 0)
     {
         printf("insert error\n");
         IVI_seg_free(seg2, NULL, NULL);
     }
     else
     {
         printf("insert success\n");
     }
     IVI_traverse(ivi_handler, print_cb, NULL);
     printf("\n");
     if(IVI_insert(ivi_handler, seg3) < 0)
     {
         printf("insert error\n");
         IVI_seg_free(seg3, NULL, NULL);
     }
     else
     {
         printf("insert success\n");
     }
     IVI_traverse(ivi_handler, print_cb, NULL);
     printf("\n");

     if(IVI_insert(ivi_handler, seg4) < 0)
     {
         printf("insert error\n");
         IVI_seg_free(seg4, NULL, NULL);
     }
     else
     {
         printf("insert success\n");
     }
     IVI_traverse(ivi_handler, print_cb, NULL);
     printf("\n");

     if(IVI_insert(ivi_handler, seg5) < 0)
     {
         printf("insert error\n");
         IVI_seg_free(seg5, NULL, NULL);
     }
     else
     {
         printf("insert success\n");
     }
     IVI_traverse(ivi_handler, print_cb, NULL);
     printf("\n");

     IVI_seg_t * prev = IVI_prev_continuous_seg(seg5);
     if(prev != NULL)
     {
         printf("prev continuous interval[%lu, %lu]\n", prev->left, prev->right);
     }
     else
     {
         printf("NULL prev continuous\n");
     }

	 IVI_seg_t * p = IVI_prev_seg(seg5);
     if(p != NULL)
     {
         printf("prev interval[%lu, %lu]\n", p->left, p->right);
     }
     else
     {
         printf("NULL prev\n");
     }

	 IVI_seg_t * next = IVI_next_continuous_seg(seg2);
     if(next != NULL)
     {
         printf("next continuous interval[%lu, %lu]\n", next->left, next->right);
     }
     else
     {
         printf("NULL nextcontinuous\n");
     }

	 IVI_seg_t * nn = IVI_next_seg(seg2);
     if(nn != NULL)
     {
         printf("next interval[%lu, %lu]\n", nn->left, nn->right);
     }
     else
     {
         printf("NULL next\n");
     }

	 IVI_seg_t * first = IVI_first_seg(ivi_handler);
	 if(first != NULL)
     {
         printf("first seg[%lu, %lu]\n", first->left, first->right);
     }
     else
     {
         printf("NULL first\n");
     }

	 IVI_seg_t * last = IVI_last_seg(ivi_handler);
	 if(last != NULL)
     {
         printf("last seg[%lu, %lu]\n", last->left, last->right);
     }
     else
     {
         printf("NULL last\n");
     }



     IVI_destroy(ivi_handler, NULL, NULL);
}

int main()
{
    insert_test();
    /* test(); */
    return 0;
}
