#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define max_sentence_length 1000       //句子最长为1000个单词
#define vocab_max_size      1000       //词汇表最多1000个单词
//词表结构
struct vocab_word {
  long long cn;//词频
  float *point;  // 记录词向量
  char *word;  //单词
  int vec_dim; //词向量维度
  int num;     //matrix时候用于平均词向量
};
struct str_and_number{
    char word[20];
    int number;
    int is_used;
};
typedef struct HuffmanNode {
    struct HuffmanNode* left;
    struct HuffmanNode* right;
    int     weight;
    char*    code;
    float*   vecor;
    int      num;
}HuffmanNode;
HuffmanNode *Node;
int ptr_str_num = 0;
struct str_and_number  str_num[10]; //快速查找所需单词信息，类似catche
struct vocab_word *vocab;//词汇表
char ***phase;            //指向训练所用短句
int cur_word_num = 0;    //词典中单词数量
int sample_num = 0;      //训练所用短句数量
int root;                //huffman树根节点位置
//定义参数
int windows = 3;
int vec_dim = 3;   //由使用者输入，为方便调试先预定义
float eta = 0.5;

void Serial_number(int* begin,int *end,int i,int j){
    int temp1,temp2;
    if(windows%2==0){           //窗口大小为偶数
        temp1 = (windows-2)/2;
        temp2 = windows/2;
    }
    else{
        temp1 = (windows-1)/2;
        temp2 = (windows-1)/2;
    }
    if(j-temp1<0){
        *begin = 0;
        *end   = windows - 1;
    }
    else if(j+temp2>=i){
        *begin = i-windows;
        *end   = i-1;
    }
    else {
        *begin = j - temp1;
        *end   = j + temp2;
    }
}
void vocab_sort(){
//按照词频升序排序
    int i,j;
    char str[50];
    int temp;
    for(i=0;i<cur_word_num;i++){
        for(j=i;j<cur_word_num;j++){
            if(vocab[j].cn>vocab[i].cn){
                strcpy(str,vocab[j].word);
                strcpy(vocab[j].word,vocab[i].word);
                strcpy(vocab[i].word,str);
                temp = vocab[j].cn;
                vocab[j].cn = vocab[i].cn;
                vocab[i].cn = temp;
            }
        }
    }
}
void add_word_to_vocab(char* str){
//将单词添加到词汇表中
    int i = 0;
    while(i<cur_word_num){
        if(vocab[i].cn==0) break; //未找到该次，加入词汇表中
        else{
            if(strcmp(str,vocab[i].word)==0){
                vocab[i].cn++;   //词典中找到该单词，词频加一
                return;
            }
            else i++ ;       //查询下一个单词是否为目标单词
        }
    }
    vocab[i].word = (char*)malloc(strlen(str)*sizeof(char));
    vocab[i].cn++;
    vocab[i].point = (float*)malloc(vec_dim*sizeof(int));
    strcpy(vocab[i].word,str);
    cur_word_num++;
    if(cur_word_num>vocab_max_size){
        printf("词汇数目超出！\n");
        return;
    }
    return;
}
void ReadFromTrain(char str[]){
//读取文本，建立表统计词频，输出训练样本
    char buffer[max_sentence_length*30]; //读取缓冲
    char *token;                         //处理缓冲
    char buffer_token[max_sentence_length][50];
//三维字符串数组存储待处理文本
    int dim1 = 5000;    //
    int dim2 = windows; //窗口大小
    int dim3 = 50;      //单词长度限制
// 使用 malloc 分配内存来创建三维字符串数组
    char ***arr = (char ***)malloc(dim1 * sizeof(char **));
    for (int i = 0; i < dim1; i++) {
        arr[i] = (char **)malloc(dim2 * sizeof(char *));
        for (int j = 0; j < dim2; j++) {
            arr[i][j] = (char *)malloc(dim3 * sizeof(char));
        }
    }
//读取文件
    FILE *fp;
    fp = fopen(str, "r");  // "r" 表示只读模式
    if (fp == NULL) {
        printf("无法打开文件.\n");
        return ;
    }
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        int i = 0;                 //去除标点符号
        while(buffer[i]){     
            if(buffer[i]=='!'||buffer[i]==','||buffer[i]=='.')
                buffer[i] = ' ';
            i++;
        }
        // 将换行符替换为空格
        for (int a = 0; buffer[a]; a++) {
            if (buffer[a] == '\n') {
                buffer[a] = ' ';
            }
        }
        token = strtok(buffer, " "); // 使用空格作为分隔符
        i = 0;
        while (token != NULL) {
            if(*token != ' '&&*token != '\n'){
                strcpy(buffer_token[i],token);
                i++;
                add_word_to_vocab(token);
            }
        // 获取下一个单词
        token = strtok(NULL, " ");
        }
        strcpy(buffer_token[i],"");   //buffer_token此时存储的是分词后的句子
        if(i<windows) continue;       //句子单词数量小于窗口大小
        int j,k;
        int begin,end;  //窗口起始与截止
        char temp[40];
        for(j=0;j<i;j++){            //字符数组第一个存储目标输出字符
            Serial_number(&begin,&end,i,j);
            for(k=0;k<windows;k++){
                strcpy(arr[sample_num][k],buffer_token[begin+k]);
            }
            strcpy(temp,arr[sample_num][0]);
            strcpy(arr[sample_num][0],arr[sample_num][j-begin]);
            strcpy(arr[sample_num][j-begin],temp);
            sample_num++;
            if (sample_num > dim1) { // 如果空间不够，重新分配
                dim1 += 5000;
                char ***temp = (char ***)realloc(arr, dim1 * sizeof(char **));
                if (temp == NULL) {
                    // 处理内存分配失败的情况
                    printf("内存分配失败\n");
                    return ;
                }
                arr = temp; // 更新外部作用域的arr指针
            }
        }
    }
    vocab_sort();//升序排序
    /////////////////////释放内存（还没写）
    fclose(fp);
    phase = arr;
    arr = NULL;
    return;
}
void vocab_init(){
    int i;
    for(i=0;i<vocab_max_size;i++){
        vocab[i].cn = 0;    //初始化词频全部为零
        vocab[i].num = 0;
        vocab[i].point = (float*)malloc(vec_dim*sizeof(float));
    }
    for(i=0;i<10;i++){
        str_num[i].is_used = 0; //快速查找结构初始化
    }
}
void print_vocab(){
    //打印词汇表
    printf("totalworld:%d\n",cur_word_num);
    int vocab_i=0;
    while(vocab_i<cur_word_num){
        printf("序号:%d  词频:%d  单词:%s  num:%d\n",vocab_i,vocab[vocab_i].cn,vocab[vocab_i].word,vocab[vocab_i].num);
        //for(int i=0;i<vec_dim;i++)
            //printf("%f  ",vocab[vocab_i].point[i]);
        printf("\n");
        vocab_i++;
    }
}
void print_sample(){
    //打印样本句子表
    int p = 0;
    while(p<sample_num){
        for(int q=0;q<windows;q++)
            printf("%s\n",phase[p][q]);
        p++;
        printf("////////////////////\n");
        }
}
void print_matrix_2(float**matrix,int row,int line){
    for(int i=0;i<row;i++){
        for(int j=0;j<line;j++)
            printf("%f  ",matrix[i][j]);
        printf("\n");
    }
    printf("finish!\n");
}
void print_matrix_3(float***matrix,int num,int row,int line){
    for(int i=1;i<num;i++){
        for(int j=0;j<row;j++){
            for(int k=0;k<line;k++){
                printf("%f ",matrix[i][j][k]);
            }
            printf("\n");
        }
        printf("矩阵打印完成！\n");
    }
    printf("全部矩阵打印完成！\n");
}
void train_matrix(){
    const float max_error=0.01;
    float err_total = 1;
//初始化隐藏层向量矩阵，矩阵维度为单词表单词数*word_vec
    float ***hid_lay_matrix = (float ***)malloc(windows * sizeof(float **));
    for (int i = 0; i < windows; i++) {
        hid_lay_matrix[i] = (float **)malloc( cur_word_num* sizeof(float *));
        for (int j = 0; j < cur_word_num; j++) {
            hid_lay_matrix[i][j] = (float *)malloc(vec_dim * sizeof(float));
        }
    }
//初始化输出层向量矩阵
    float **output_lay_matrix = (float **)malloc(vec_dim * sizeof(float *));
    for (int i = 0; i < vec_dim; i++) {
        output_lay_matrix[i] = (float *)malloc(cur_word_num * sizeof(float));
    }
//隐藏矩阵，输出矩阵
    float* hid_layer;
    float* output_layer;
    hid_layer = (float*)malloc(vec_dim*sizeof(float));
    output_layer = (float*)malloc(cur_word_num*sizeof(float));
    for(int i=0;i<vec_dim;i++)
        hid_layer[i] = 0;
    for(int i=0;i<cur_word_num;i++)
        output_layer[i] = 0;

    // srand(time(NULL)); //随机数填充矩阵
    for (int k = 0; k < windows; k++) {
        for (int i = 0; i < cur_word_num; i++) {
            for (int j = 0; j < vec_dim; j++) {
                hid_lay_matrix[k][i][j] = 1.0 * rand() / RAND_MAX; // 生成0到1之间的随机浮点数
            }
        }
    }
    for (int k = 0; k < vec_dim; k++) {
        for (int i = 0; i < cur_word_num; i++) {
            output_lay_matrix[k][i] = 1.0 * rand() / RAND_MAX; // 生成0到1之间的随机浮点数
        }
    }
    //print_matrix_2(output_lay_matrix,vec_dim,cur_word_num);
    //print_matrix_3(hid_lay_matrix,windows,cur_word_num,vec_dim);
    int trained_num_sample = 0;
    int word_number[50];   //每个单词对应词典位置编号,需要窗口大小小于50
    int flag;              //快速查找块上是否找到所需单词
    //print_sample();
    //print_vocab();

    float* EH;
    EH = (float*)malloc(vec_dim*sizeof(float));   //存储误差
    for(int i=0;i<vec_dim;i++)
        EH[i] = 0;
    float* error;
    error = (float*)malloc(cur_word_num*sizeof(float));
    while(trained_num_sample < sample_num){
        while(err_total>max_error){
            err_total = 0;
            //找到每个窗口单词对应的词典单词编号
            for(int i=0;i<windows;i++){
                flag = 0;
                for(int j=0;j<10;j++){
                    if( strcmp(str_num[j].word,phase[trained_num_sample][i])==0 ){
                        word_number[i] = str_num[j].number;    //最近访问的单词中找到所需单词
                        flag = 1;
                        break;
                    }
                }
                if(flag==0){
                    //未找到所需单词序号
                    for(int k=0;k<cur_word_num;k++){
                        if(strcmp(vocab[k].word,phase[trained_num_sample][i])==0 ){
                            word_number[i] = k;
                            str_num[ptr_str_num].is_used = 1;   //更改快速查找块
                            str_num[ptr_str_num].number = k;
                            strcpy(str_num[ptr_str_num].word,phase[trained_num_sample][i]);
                            ptr_str_num = (ptr_str_num+1)%10;
                            break;
                        }
                    }
                }
            }
            // for(int i=0;i<windows;i++){  
            //     //打印待处理单词的词典编号
            //     printf("序号:%d 单词:%s\n",word_number[i],phase[trained_num_sample][i]);
            // }
            //隐藏层相乘得出隐藏层向量
            for(int i=0;i<vec_dim;i++){
                for(int j=1;j<windows;j++)
                    hid_layer[i] +=  hid_lay_matrix[j][word_number[j]][i];
                hid_layer[i] = hid_layer[i]/(windows-1);
                //printf("hid_layer[%d]=%f\n",i,hid_layer[i]);
            }
            //算出输出层矩阵
            for(int i=0;i<cur_word_num;i++){
                for(int j=0;j<vec_dim;j++)
                    output_layer[i] += hid_layer[j]*output_lay_matrix[j][i];
                //printf("output_layer[%d]=%f\n",i,output_layer[i]);
            }
            //softmax归一化
            float total=0;
            for(int i=0;i<cur_word_num;i++){
                output_layer[i] = exp(output_layer[i]);
                total += output_layer[i];
            }
            for(int i=0;i<cur_word_num;i++){
                output_layer[i] = output_layer[i]/total;
                //printf("%f\n",output_layer[i]);
            }
            //计算总误差
            int k;
            for(int i=0;i<cur_word_num;i++){
                if(i==word_number[0])
                    k = 1;
                else 
                    k = 0;
                error[i] = output_layer[i]-k;
                //printf("error[%d]:%f\n",i,error[i]);
                err_total += pow(output_layer[i]-k,2);
            }
            //更新输出层矩阵
            int a;
            for(int i=0;i<vec_dim;i++){
                for(int j=0;j<cur_word_num;j++){
                    output_lay_matrix[i][j] = output_lay_matrix[i][j] - eta*error[j]*hid_layer[i];
                }
            }
            //更新隐藏层矩阵
            for(int i=0;i<vec_dim;i++){
                for(int j=0;j<cur_word_num;j++){
                    EH[i] += error[j]*output_lay_matrix[i][j];
                }
                //printf("EH[%d]:%f\n",i,EH[i]);
            }
            for(int i=1;i<windows;i++){
                for(int j=0;j<cur_word_num;j++){
                    if(word_number[i]==j){
                        for(int k=0;k<vec_dim;k++){
                            hid_lay_matrix[i][j][k] = hid_lay_matrix[i][j][k] - eta*EH[k]/(windows-1);
                        }
                    }
                }
            }
        //printf("//////////////\n");
        //printf("trained_num_sample:%d  current error:%f\n",trained_num_sample,err_total);
        }
    // printf("trained_num_sample:%d\n",trained_num_sample);
    // for(int i=0;i<cur_word_num;i++)
    //     printf("%f\n",output_layer[i]);
    //  printf("\n");
    //  printf("//////////////////////");
    //  printf("\n");
    //添加词向量到词典中
    for(int i=1;i<windows;i++){ 
        for(int j=0;j<vec_dim;j++)
            vocab[word_number[i]].point[j] = hid_lay_matrix[i][word_number[i]][j];
        vocab[word_number[i]].num++;
    }
    err_total = 1;
    trained_num_sample++;
    for(int i=0;i<vec_dim;i++)
        EH[i] = 0;
    for(int i=0;i<vec_dim;i++)
        hid_layer[i] = 0;
    for(int i=0;i<cur_word_num;i++)
        output_layer[i] = 0;
    }
    for(int i=0;i<cur_word_num;i++){ //取平均值
        for(int j=0;j<vec_dim;j++)
            vocab[i].point[j] = (vocab[i].point[j])/(vocab[i].num);
    }
// 释放内存
}
void Node_select(int num_leaf,int* num1,int*num2,int is_selected[]){
    int weight1 = max_sentence_length,weight2 = max_sentence_length;
    for(int i=0;i<2*num_leaf-1;i++){ //找出最小值
        if(weight1>=Node[i].weight&&is_selected[i]==0&&Node[i].weight!=0){
            *num1 = i;
            weight1 = Node[i].weight;
        }
    }
    for(int i=0;i<2*num_leaf-1;i++){ //找出次小值
        if(weight2>=Node[i].weight&&is_selected[i]==0&&Node[i].weight!=0&&i!=*num1){
            *num2 = i;
            weight2 = Node[i].weight;
        }
    }
    is_selected[*num1] = 1;
    is_selected[*num2] = 1;
}
void InitLeaf(){ //置零
    for(int i=0;i<2*cur_word_num-1;i++){
        Node[i].weight = 0;
        Node[i].left = NULL;
        Node[i].right = NULL;
        Node[i].code = (char*)malloc(cur_word_num*sizeof(char));
        for(int j=0;j<cur_word_num;j++)
            Node[i].code[j] = '\0';
    }
    for(int i=0;i<2*cur_word_num-1;i++){
        Node[i].num = 0;
        Node[i].vecor = (float*)malloc(vec_dim*sizeof(float));
        for(int j=0;j<vec_dim;j++){
            //Node[i].vecor[j] = 1.0 * rand() / RAND_MAX;
            Node[i].vecor[j] = 0;
            //printf("%f  ",Node[i].vecor[j]);
        }
        //printf("\n");
    }
}
void create_HuffmanTree(){
    int num_leaf = cur_word_num;
    int num1,num2;
    int is_selected[2*cur_word_num-1];
    int ptr = cur_word_num;
    Node = (HuffmanNode*)malloc((2*num_leaf-1)*sizeof(HuffmanNode));
    //初始化所有节点
    InitLeaf();  
    //初始化叶子结点
    for(int i=0;i<num_leaf;i++)
        Node[i].weight = vocab[i].cn;
    for(int i=0;i<2*cur_word_num-1;i++)  //辅助数组
        is_selected[i] = 0;
    for(int i=num_leaf;i<2*num_leaf-1;i++){ 
        Node_select(num_leaf,&num1,&num2,is_selected);//挑选两个权值最小的节点
        Node[ptr].weight = Node[num1].weight+Node[num2].weight;
        Node[ptr].left = &Node[num1];
        Node[ptr].right = &Node[num2];
        ptr++;
    }
    root = ptr-1;
    return;
}
void create_HuffmanCode(HuffmanNode root,char* code){
    int i=0;
    strcpy(root.code,code);
    while(code[i]!='\0')
        i++; 
    if(root.left!=NULL){
        code[i] = '1';
        create_HuffmanCode(*root.left,code);
    }
    if(root.right!=NULL){
        code[i] = '0';
        create_HuffmanCode(*root.right,code);
    }
    code[i] = '\0';
    return;
}
void print_HuffmanCode(){
    for(int i=0;i<cur_word_num;i++)
        printf("序号:%d code:%s\n",i,Node[i].code);
}
float sigmoid(float num){
    return 1/(1+exp(-num));
}
void train_HuffmanTree(){
    //print_sample();
    //print_vocab();
    const float base_prob = 0.95;
    float  cur_prob = 0;
    char*  code;
    int    word_number[50];   //每个单词对应词典位置编号,需要窗口大小小于50
    int    flag;              //快速查找块上是否找到所需单词
    float* input;             //输入向量X
    int    digits = 0;            //拐弯的次数
    float* update_leafNode;   //存储叶子结点向量更新
    update_leafNode = (float*)malloc(vec_dim*sizeof(float));
    for(int i=0;i<vec_dim;i++)
        update_leafNode[i] = 0;
    input = (float*)malloc(vec_dim*sizeof(float));
    for(int i=0;i<vec_dim;i++)      //置空
        input[i] = 0;
    code = (char*)malloc(cur_word_num*sizeof(char));
    for(int i=0;i<cur_word_num;i++) //置空
        code[i] = '\0';
    create_HuffmanTree();  //创建huffman树
    create_HuffmanCode(Node[2*cur_word_num-2],code);  //根据huffman树编码
    //print_HuffmanCode();
    int trained_num_sample = 0;
    while(trained_num_sample<sample_num){
        //找到每个窗口单词对应的词典单词编号
        for(int i=0;i<windows;i++){
            flag = 0;
            for(int j=0;j<10;j++){
                if( strcmp(str_num[j].word,phase[trained_num_sample][i])==0 ){
                    word_number[i] = str_num[j].number;    //最近访问的单词中找到所需单词
                    flag = 1;
                    break;
                }
            }
            if(flag==0){
                //未找到所需单词序号
                for(int k=0;k<cur_word_num;k++){
                    if(strcmp(vocab[k].word,phase[trained_num_sample][i])==0 ){
                        word_number[i] = k;
                        str_num[ptr_str_num].is_used = 1;   //更改快速查找块
                        str_num[ptr_str_num].number = k;
                        strcpy(str_num[ptr_str_num].word,phase[trained_num_sample][i]);
                        ptr_str_num = (ptr_str_num+1)%10;
                        break;
                    }
                }
            }
        }
        for(int i=0;i<windows;i++){  
             //打印待处理单词的词典编号
             //printf("序号:%d 单词:%s\n",word_number[i],phase[trained_num_sample][i]);
        }
        //算出输入向量
        // for(int i=0;i<vec_dim;i++){
        //     input[i] = 0;
        //     for(int j=1;j<windows;j++)
        //         input[i] += Node[word_number[j]].vecor[i];
        //     input[i] = input[i]/(windows-1);
        // }
        for(int i=0;i<vec_dim;i++)
            input[i] = 1.0 * rand() / RAND_MAX;
        //huffman节点初始化
        for(int i=cur_word_num;i<2*cur_word_num-1;i++)
            for(int j=0;j<vec_dim;j++)
                Node[i].vecor[j] = 1.0 * rand() / RAND_MAX;
        
        while(cur_prob<base_prob){
            HuffmanNode *curr_Node;
            cur_prob = 1;
            float  matrix_mul = 0;
            float  matrix_mul_result[max_sentence_length];
            //计算当前概率
            while(Node[word_number[0]].code[digits]!='\0')  
                digits++;
            int pos = 0;
            curr_Node = &Node[root];
            while(pos<digits){
                //矩阵相乘
                matrix_mul = 0;
                for(int i=0;i<vec_dim;i++)
                    matrix_mul += curr_Node->vecor[i]*input[i];
                matrix_mul_result[pos] = sigmoid(matrix_mul);   //记录每步矩阵相乘结果
                if(Node[word_number[0]].code[pos]=='1'){
                    cur_prob = cur_prob*(1-sigmoid(matrix_mul));
                    curr_Node = curr_Node->left;
                }
                else{   
                    cur_prob = cur_prob*sigmoid(matrix_mul);
                    curr_Node = curr_Node->right;
                }
                pos++;
            }
            //huffman节点更新参数
            curr_Node = &Node[root]; 
            int djw;
            pos = 0;
            while(pos<digits){
                if(Node[word_number[0]].code[pos]=='1') djw = 1;
                    else djw = 0;
                for(int i=0;i<vec_dim;i++)
                    curr_Node->vecor[i] += eta*(1-djw-matrix_mul_result[pos])*input[i];
                if(djw)
                    curr_Node = curr_Node->left;
                else
                    curr_Node = curr_Node->right;
                pos++;
            }
            //词向量更新
            curr_Node = &Node[root]; 
            pos = 0;
            for(int i=0;i<vec_dim;i++)
                update_leafNode[i] = 0;
            while(pos<digits){
                if(Node[word_number[0]].code[pos]=='1') djw = 1;
                    else djw = 0;
                for(int i=0;i<vec_dim;i++)
                    update_leafNode[i] += (1-djw-matrix_mul_result[pos])*curr_Node->vecor[i];
                if(djw)
                    curr_Node = curr_Node->left;
                else
                    curr_Node = curr_Node->right;
                pos++;
            }
            for(int i=1;i<windows;i++){
                for(int j=0;j<vec_dim;j++)
                    Node[word_number[i]].vecor[j] += update_leafNode[j];
            }
            for(int i=0;i<vec_dim;i++)
                input[i] += update_leafNode[i];
                //printf("cur_prob:%f\n",cur_prob);
        }
        for(int i=0;i<windows;i++)
            Node[word_number[i]].num++;
        // printf("\\\\\\\\\\\\\\\\\\\\\\\\");
        // putchar('\n');
        //printf("number:%d,prob:%f\n",trained_num_sample,cur_prob);
        // printf("\\\\\\\\\\\\\\\\\\\\\\\\");
        // putchar('\n');
        cur_prob = 0;
        digits = 0;
        trained_num_sample++;
    }
    for(int i=0;i<cur_word_num;i++){
        for(int j=0;j<vec_dim;j++){
            vocab[i].point[j] = Node[i].vecor[j]/(Node[i].num);
            //printf("%f  ",vocab[i].point[j]);
        }
        //printf("\n");
    }
}
int main(){
    srand(time(NULL)); //随机数种子
    //scanf("%d",&vec_dim);
    vocab = (struct vocab_word *)malloc(vocab_max_size*sizeof(struct vocab_word));
    vocab_init();
    ReadFromTrain("data.txt");
    //train_matrix();
    train_HuffmanTree();
    //print_vocab();
    free(vocab);
    vocab = NULL;
}