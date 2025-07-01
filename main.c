#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MIN_DEGREE 2
#define MAX_KEYS (2 * MIN_DEGREE - 1)    
#define MIN_KEYS (MIN_DEGREE - 1)        
#define MAX_CHILDREN (2 * MIN_DEGREE)    

/* Tipos de nó: Arquivo ou Diretório */
typedef enum { FILE_TYPE, DIRECTORY_TYPE } NodeType;

/* Estrutura para representar um arquivo */
typedef struct File {
    char* name;
    char* content;
    size_t size;
} File;

/* Declaração antecipada das estruturas Directory e BTree */
struct Directory;
typedef struct Directory Directory;
struct BTree;
typedef struct BTree BTree;

/* Estrutura para representar um nó genérico (arquivo ou diretório) na árvore de arquivos */
typedef struct TreeNode {
    char* name;
    NodeType type;
    union {
        File* file;
        Directory* directory;
    } data;
} TreeNode;

/* Estrutura para representar um diretório */
struct Directory {
    BTree* tree;         
    Directory* parent;   
    char* name;          
};

/* Estrutura de um nó da Árvore B */
typedef struct BTreeNode {
    bool folha;                           
    int n;                                
    TreeNode* chaves[MAX_KEYS];           
    struct BTreeNode* filhos[MAX_CHILDREN]; 
} BTreeNode;

/* Estrutura da Árvore B */
struct BTree {
    BTreeNode* raiz;    
    int t;              
};

/* Cria um novo nó BTreeNode (folha ou interno) */
BTreeNode* btree_node_create(bool folha) {
    BTreeNode* node = (BTreeNode*) malloc(sizeof(BTreeNode));
    if (!node) {
        fprintf(stderr, "Erro de alocação de memória ao criar nó B-Tree.\n");
        exit(EXIT_FAILURE);
    }
    node->folha = folha;
    node->n = 0;

    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->filhos[i] = NULL;
    }
    return node;
}

/* Inicializa uma nova árvore B vazia e retorna seu ponteiro */
BTree* btree_create() {
    BTree* tree = (BTree*) malloc(sizeof(BTree));
    if (!tree) {
        fprintf(stderr, "Erro de alocação de memória ao criar árvore B.\n");
        exit(EXIT_FAILURE);
    }
    tree->t = MIN_DEGREE;

    tree->raiz = btree_node_create(true);
    return tree;
}

/* Função recursiva para buscar uma chave (nome) na subárvore enraizada no nó x */
TreeNode* btree_search_node(BTreeNode* x, const char* name) {
    int i = 0;
    
    while (i < x->n && strcmp(name, x->chaves[i]->name) > 0) {
        i++;
    }
    if (i < x->n && strcmp(name, x->chaves[i]->name) == 0) {
        return x->chaves[i];
    }
    if (x->folha) {
        return NULL;
    }
    return btree_search_node(x->filhos[i], name);
}

/* Busca uma chave na árvore B a partir da raiz */
TreeNode* btree_search(BTree* tree, const char* name) {
    if (!tree || !tree->raiz) return NULL;
    return btree_search_node(tree->raiz, name);
}

/* Divide o filho y do nó x em dois, quando y está cheio. 
   i é o índice de y em x->filhos antes da divisão. */
void btree_split_child(BTreeNode* x, int i, BTreeNode* y) {
    BTreeNode* z = btree_node_create(y->folha);
    int t = MIN_DEGREE;
    z->n = t - 1;
    for (int j = 0; j < t - 1; j++) {
        z->chaves[j] = y->chaves[j + t];
    }
    if (!y->folha) {
        for (int j = 0; j < t; j++) {
            z->filhos[j] = y->filhos[j + t];
        }
    }

    y->n = t - 1;
    for (int j = x->n; j >= i + 1; j--) {
        x->filhos[j + 1] = x->filhos[j];
    }
    x->filhos[i + 1] = z;
    for (int j = x->n - 1; j >= i; j--) {
        x->chaves[j + 1] = x->chaves[j];
    }
    x->chaves[i] = y->chaves[t - 1];
    x->n += 1;
}

/* Insere o TreeNode *novo em um nó (subárvore) que *não* está cheio */
void btree_insert_nonfull(BTreeNode* x, TreeNode* novo) {
    int i = x->n - 1;
    if (x->folha) {
        while (i >= 0 && strcmp(novo->name, x->chaves[i]->name) < 0) {
            x->chaves[i + 1] = x->chaves[i];
            i--;
        }
        x->chaves[i + 1] = novo;
        x->n += 1;
    } else {
        while (i >= 0 && strcmp(novo->name, x->chaves[i]->name) < 0) {
            i--;
        }
        i++;
        if (x->filhos[i]->n == MAX_KEYS) {
            btree_split_child(x, i, x->filhos[i]);
            if (strcmp(novo->name, x->chaves[i]->name) > 0) {
                i++;
            }
        }
        btree_insert_nonfull(x->filhos[i], novo);
    }
}

/* Insere um TreeNode (arquivo ou diretório) na árvore B do diretório */
bool btree_insert(BTree* tree, TreeNode* novo) {
    BTreeNode* r = tree->raiz;
    if (r->n == MAX_KEYS) {
        BTreeNode* s = btree_node_create(false);
        s->filhos[0] = r;
        btree_split_child(s, 0, r);
        int i = 0;
        if (strcmp(novo->name, s->chaves[0]->name) > 0) {
            i = 1;
        }
        btree_insert_nonfull(s->filhos[i], novo);
        tree->raiz = s; 
    } else {
        btree_insert_nonfull(r, novo);
    }
    return true;
}

/* Busca o predecessor (TreeNode com a maior chave menor que chaves[idx]) */
TreeNode* btree_get_predecessor(BTreeNode* x, int idx) {
    BTreeNode* cur = x->filhos[idx];

    while (!cur->folha) {
        cur = cur->filhos[cur->n];
    }
    return cur->chaves[cur->n - 1]; 
}

/* Busca o sucessor (TreeNode com a menor chave maior que chaves[idx]) */
TreeNode* btree_get_successor(BTreeNode* x, int idx) {
    BTreeNode* cur = x->filhos[idx + 1];

    while (!cur->folha) {
        cur = cur->filhos[0];
    }
    return cur->chaves[0];
}

/* Empresta uma chave do irmão esquerdo (filho idx-1) para o filho idx de x */
void btree_borrow_from_prev(BTreeNode* x, int idx) {
    BTreeNode* child = x->filhos[idx];
    BTreeNode* sibling = x->filhos[idx - 1];

    for (int j = child->n - 1; j >= 0; --j) {
        child->chaves[j + 1] = child->chaves[j];
    }

    if (!child->folha) {
        for (int j = child->n; j >= 0; --j) {
            child->filhos[j + 1] = child->filhos[j];
        }
    }

    child->chaves[0] = x->chaves[idx - 1];

    if (!child->folha) {
        child->filhos[0] = sibling->filhos[sibling->n];
    }

    x->chaves[idx - 1] = sibling->chaves[sibling->n - 1];
    child->n += 1;
    sibling->n -= 1;
}

/* Empresta uma chave do irmão direito (filho idx+1) para o filho idx de x */
void btree_borrow_from_next(BTreeNode* x, int idx) {
    BTreeNode* child = x->filhos[idx];
    BTreeNode* sibling = x->filhos[idx + 1];

    child->chaves[child->n] = x->chaves[idx];

    if (!child->folha) {
        child->filhos[child->n + 1] = sibling->filhos[0];
    }

    x->chaves[idx] = sibling->chaves[0];

    for (int j = 1; j < sibling->n; ++j) {
        sibling->chaves[j - 1] = sibling->chaves[j];
    }

    if (!sibling->folha) {
        for (int j = 1; j <= sibling->n; ++j) {
            sibling->filhos[j - 1] = sibling->filhos[j];
        }
    }
    child->n += 1;
    sibling->n -= 1;
}

/* Funde o filho idx com o filho idx+1 de x, movendo a chave de x[idx] para o novo nó unido */
void btree_merge_children(BTreeNode* x, int idx) {
    BTreeNode* child = x->filhos[idx];
    BTreeNode* sibling = x->filhos[idx + 1];
    int t = MIN_DEGREE;

    child->chaves[t - 1] = x->chaves[idx];
    for (int j = 0; j < sibling->n; j++) {
        child->chaves[t + j] = sibling->chaves[j];
    }
    if (!child->folha) {
        for (int j = 0; j <= sibling->n; j++) {
            child->filhos[t + j] = sibling->filhos[j];
        }
    }
    child->n += sibling->n + 1;
    for (int j = idx; j < x->n - 1; j++) {
        x->chaves[j] = x->chaves[j + 1];
    }
    for (int j = idx + 1; j < x->n; j++) {
        x->filhos[j] = x->filhos[j + 1];
    }
    x->n -= 1;
    free(sibling);
}

/* Remove recursivamente a chave 'name' da subarvore enraizada em x (assume que a chave existe na árvore) */
void btree_delete_from_node(BTreeNode* x, const char* name) {
    int idx = 0;
    while (idx < x->n && strcmp(name, x->chaves[idx]->name) > 0) {
        idx++;
    }
    if (idx < x->n && strcmp(name, x->chaves[idx]->name) == 0) {
        if (x->folha) {
            for (int j = idx; j < x->n - 1; j++) {
                x->chaves[j] = x->chaves[j + 1];
            }
            x->n -= 1;
        } else {
            BTreeNode* y = x->filhos[idx];       
            BTreeNode* z = x->filhos[idx + 1];   
            if (y->n >= MIN_DEGREE) {
                TreeNode* pred = btree_get_predecessor(x, idx);
                x->chaves[idx] = pred;  
                btree_delete_from_node(y, pred->name); 
            } else if (z->n >= MIN_DEGREE) {
                TreeNode* succ = btree_get_successor(x, idx);
                x->chaves[idx] = succ;
                btree_delete_from_node(z, succ->name);
            } else {
                btree_merge_children(x, idx);
                btree_delete_from_node(y, name);
            }
        }
    } else {
        if (x->folha) {
            return;
        }

        bool ultimoFilho = (idx == x->n);
        BTreeNode* filhoAlvo = x->filhos[idx];
        
        if (filhoAlvo->n == MIN_KEYS) {
            if (idx > 0 && x->filhos[idx - 1]->n >= MIN_DEGREE) {
                btree_borrow_from_prev(x, idx);
            } else if (idx < x->n && x->filhos[idx + 1]->n >= MIN_DEGREE) {
                btree_borrow_from_next(x, idx);
            } else {
                if (idx < x->n) {
                    btree_merge_children(x, idx);
                } else {
                    btree_merge_children(x, idx - 1);
                    filhoAlvo = x->filhos[idx - 1];
                }
            }
        }
        if (ultimoFilho && idx > x->n) {
            btree_delete_from_node(x->filhos[idx - 1], name);
        } else {
            btree_delete_from_node(x->filhos[idx], name);
        }
    }
}

/* Remove uma chave da árvore B (se existir) e retorna o TreeNode removido ou NULL */
TreeNode* btree_delete(BTree* tree, const char* name) {
    if (!tree || !tree->raiz) return NULL;
    BTreeNode* r = tree->raiz;
    TreeNode* removido = btree_search(tree, name);
    if (removido == NULL) {
        return NULL;  
    }

    btree_delete_from_node(r, name);
    if (r->n == 0) {
        if (!r->folha) {
            tree->raiz = r->filhos[0];
        }
        if (r != tree->raiz) {
            free(r);
        }
    }
    return removido;
}

/* Percorre a árvore B e imprime os nomes das entradas em ordem (uso em listagem) */
void btree_traverse_node(BTreeNode* x) {
    int i;
    for (i = 0; i < x->n; i++) {
        if (!x->folha) {
            btree_traverse_node(x->filhos[i]);
        }
        if (x->chaves[i]->type == DIRECTORY_TYPE) {
            printf("%s/\n", x->chaves[i]->name);
        } else {
            printf("%s\n", x->chaves[i]->name);
        }
    }
    if (!x->folha) {
        btree_traverse_node(x->filhos[i]);
    }
}

/* Interface pública para percorrer a árvore B de um diretório e listar seu conteúdo */
void btree_traverse(BTree* tree) {
    if (tree != NULL && tree->raiz != NULL) {
        btree_traverse_node(tree->raiz);
    }
}

/* Libera recursivamente todos os nós de uma árvore B (auxiliar para destruir diretório) */
void btree_free_node(BTreeNode* x) {
    if (!x->folha) {
        for (int i = 0; i <= x->n; i++) {
            if (x->filhos[i] != NULL) {
                btree_free_node(x->filhos[i]);
            }
        }
    }
    free(x);
}

/* Libera toda a estrutura da árvore B */
void btree_destroy(BTree* tree) {
    if (!tree) return;
    if (tree->raiz != NULL) {
        btree_free_node(tree->raiz);
    }
    free(tree);
}

/* Cria um novo TreeNode de arquivo .txt com nome e conteúdo especificados */
TreeNode* create_txt_file_node(const char* name, const char* content) {
    // Aloca e configura a estrutura File
    File* file = (File*) malloc(sizeof(File));
    if (!file) {
        fprintf(stderr, "Erro de alocação de memória para arquivo.\n");
        return NULL;
    }
    file->name = (char*) malloc(strlen(name) + 1);
    if (!file->name) {
        fprintf(stderr, "Erro de alocação de memória para nome do arquivo.\n");
        free(file);
        return NULL;
    }
    strcpy(file->name, name);
    file->size = content ? strlen(content) : 0;
    if (content) {
        if (file->size > 1024 * 1024) {
            fprintf(stderr, "Conteúdo excede o tamanho máximo de 1MB.\n");
            free(file->name);
            free(file);
            return NULL;
        }
        file->content = (char*) malloc(file->size + 1);
        if (!file->content) {
            fprintf(stderr, "Erro de alocação ao copiar conteúdo do arquivo.\n");
            free(file->name);
            free(file);
            return NULL;
        }
        strcpy(file->content, content);
    } else {
        file->content = NULL;
    }
    TreeNode* node = (TreeNode*) malloc(sizeof(TreeNode));
    if (!node) {
        fprintf(stderr, "Erro de alocação de memória para TreeNode de arquivo.\n");
        free(file->name);
        if (file->content) free(file->content);
        free(file);
        return NULL;
    }
    node->name = file->name;
    node->type = FILE_TYPE;
    node->data.file = file;
    return node;
}

/* Cria um novo TreeNode de diretório com o nome especificado */
TreeNode* create_directory_node(const char* name, Directory* parent) {
    Directory* dir = (Directory*) malloc(sizeof(Directory));
    if (!dir) {
        fprintf(stderr, "Erro de alocação de memória para Directory.\n");
        return NULL;
    }
    dir->parent = parent;
    dir->tree = btree_create(); 
    TreeNode* node = (TreeNode*) malloc(sizeof(TreeNode));
    if (!node) {
        fprintf(stderr, "Erro de alocação de memória para TreeNode de diretório.\n");
        btree_destroy(dir->tree);
        free(dir);
        return NULL;
    }
    char* nomeCopia = (char*) malloc(strlen(name) + 1);
    if (!nomeCopia) {
        fprintf(stderr, "Erro de alocação de memória para nome do diretório.\n");
        btree_destroy(dir->tree);
        free(dir);
        return NULL;
    }
    strcpy(nomeCopia, name);
    dir->name = nomeCopia;
    node->name = nomeCopia;
    node->type = DIRECTORY_TYPE;
    node->data.directory = dir;
    return node;
}

/* Insere (cria) um novo arquivo .txt no diretório atual */
bool create_txt_file(Directory* currentDir, const char* name, const char* content) {
    const char* ext = strrchr(name, '.');
    if (!ext || strcmp(ext, ".txt") != 0) {
        printf("Erro: apenas arquivos .txt podem ser criados.\n");
        return false;
    }
    if (btree_search(currentDir->tree, name) != NULL) {
        printf("Erro: já existe um arquivo ou diretório com o nome \"%s\".\n", name);
        return false;
    }
    TreeNode* node = create_txt_file_node(name, content);
    if (!node) {
        printf("Erro ao criar arquivo \"%s\".\n", name);
        return false;
    }
    btree_insert(currentDir->tree, node);
    return true;
}

/* Insere (cria) um novo diretório no diretório atual */
bool create_directory(Directory* currentDir, const char* name) {
    if (btree_search(currentDir->tree, name) != NULL) {
        printf("Erro: já existe um arquivo ou diretório com o nome \"%s\".\n", name);
        return false;
    }
    TreeNode* node = create_directory_node(name, currentDir);
    if (!node) {
        printf("Erro ao criar diretório \"%s\".\n", name);
        return false;
    }
    btree_insert(currentDir->tree, node);
    return true;
}

/* Remove um arquivo .txt do diretório atual */
bool delete_txt_file(Directory* currentDir, const char* name) {
    TreeNode* node = btree_search(currentDir->tree, name);
    if (node == NULL) {
        printf("Erro: arquivo \"%s\" não encontrado.\n", name);
        return false;
    }
    if (node->type != FILE_TYPE) {
        printf("Erro: \"%s\" não é um arquivo.\n", name);
        return false;
    }
    TreeNode* removido = btree_delete(currentDir->tree, name);
    if (!removido) {
        printf("Erro ao remover arquivo \"%s\".\n", name);
        return false;
    }
    File* file = removido->data.file;
    free(file->content);
    free(file);
    free(removido->name);
    free(removido);
    return true;
}

/* Remove um diretório vazio do diretório atual */
bool delete_directory(Directory* currentDir, const char* name) {
    TreeNode* node = btree_search(currentDir->tree, name);
    if (node == NULL) {
        printf("Erro: diretório \"%s\" não encontrado.\n", name);
        return false;
    }
    if (node->type != DIRECTORY_TYPE) {
        printf("Erro: \"%s\" não é um diretório.\n", name);
        return false;
    }
    Directory* dir = node->data.directory;
    if (dir->tree->raiz->n != 0) {
        printf("Erro: diretório \"%s\" não está vazio.\n", name);
        return false;
    }
    if (dir->parent == NULL) {
        printf("Erro: não é permitido remover o diretório raiz.\n");
        return false;
    }
    
    TreeNode* removido = btree_delete(currentDir->tree, name);
    if (!removido) {
        printf("Erro ao remover diretório \"%s\".\n", name);
        return false;
    }
    btree_destroy(dir->tree);
    free(removido->name);
    free(dir);
    free(removido);
    return true;
}

/* Altera o diretório atual (simulação do comando 'cd') */
Directory* change_directory(Directory* currentDir, const char* name) {
    if (strcmp(name, "..") == 0) {
        if (currentDir->parent != NULL) {
            return currentDir->parent;
        } else {
            return currentDir;
        }
    } else if (strcmp(name, "/") == 0) {
        Directory* temp = currentDir;
        while (temp->parent != NULL) {
            temp = temp->parent;
        }
        return temp;
    } else {
        TreeNode* node = btree_search(currentDir->tree, name);
        if (node == NULL || node->type != DIRECTORY_TYPE) {
            printf("Erro: diretório \"%s\" não encontrado.\n", name);
            return currentDir;
        }
        return node->data.directory;
    }
}

/* Lista o conteúdo do diretório atual (arquivos e subdiretórios) */
void list_directory_contents(Directory* currentDir) {
    BTreeNode* root = currentDir->tree->raiz;
    if (root->n == 0) {
        printf("[Diretório vazio]\n");
    } else {
        btree_traverse(currentDir->tree);
    }
}

/* Função auxiliar recursiva para imprimir entradas em ordem no arquivo de imagem */
void print_entries_rec(BTreeNode* node, int indent, FILE* f) {
    for (int i = 0; i < node->n; i++) {
        if (!node->folha) {
            print_entries_rec(node->filhos[i], indent, f);
        }
        for (int j = 0; j < indent; j++) {
            fprintf(f, "    ");
        }
        TreeNode* entry = node->chaves[i];
        if (entry->type == DIRECTORY_TYPE) {
            fprintf(f, "%s/\n", entry->name);
            print_entries_rec(entry->data.directory->tree->raiz, indent + 1, f);
        } else {
            fprintf(f, "%s (tamanho=%zu bytes)\n", entry->name, entry->data.file->size);
        }
    }
    if (!node->folha) {
        print_entries_rec(node->filhos[node->n], indent, f);
    }
}

/* Salva a estrutura do sistema de arquivos em um arquivo texto (imagem) */
void save_filesystem_image(Directory* rootDir, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("Erro: não foi possível criar o arquivo de imagem do sistema de arquivos.\n");
        return;
    }
    fprintf(f, "/\n");
    print_entries_rec(rootDir->tree->raiz, 1, f);
    fclose(f);
}


int main() {
    Directory* root = (Directory*) malloc(sizeof(Directory));
    root->parent = NULL;
    root->tree = btree_create();
    root->name = NULL;
    Directory* current = root;
    char input[4096];
    printf("Sistema de Arquivos Virtual iniciado. Diretório atual: raiz (/) \n");
    printf("Comandos disponíveis: criar_arquivo <nome.txt> <conteudo>, criar_pasta <nome>, ");
    printf("remover_arquivo <nome.txt>, remover_pasta <nome>, cd <dir>, cd .., ls, sair\n");

    while (true) {
        printf("\n%s> ", (current->parent == NULL ? "/" : current->name));
        if (!fgets(input, sizeof(input), stdin)) {
            break; 
        }
        
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        if (strlen(input) == 0) {
            continue;
        }
        char cmd[50];
        char arg1[1024];
        char arg2[2048];
        int count = sscanf(input, "%49s %1023s %2047[^\n]", cmd, arg1, arg2);
        if (count <= 0) {
            continue;
        }
        if (strcmp(cmd, "sair") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "ls") == 0) {
            list_directory_contents(current);
        } else if (strcmp(cmd, "cd") == 0) {
            if (count >= 2) {
                current = change_directory(current, arg1);
            } else {
                printf("Uso: cd <diretorio>\n");
            }
        } else if (strcmp(cmd, "criar_pasta") == 0 || strcmp(cmd, "mkdir") == 0) {
            if (count >= 2) {
                create_directory(current, arg1);
            } else {
                printf("Uso: criar_pasta <nome>\n");
            }
        } else if (strcmp(cmd, "remover_pasta") == 0 || strcmp(cmd, "rmdir") == 0) {
            if (count >= 2) {
                delete_directory(current, arg1);
            } else {
                printf("Uso: remover_pasta <nome>\n");
            }
        } else if (strcmp(cmd, "criar_arquivo") == 0 || strcmp(cmd, "touch") == 0) {
            if (count >= 3) {
                create_txt_file(current, arg1, arg2);
            } else {
                printf("Uso: criar_arquivo <nome.txt> <conteudo>\n");
            }
        } else if (strcmp(cmd, "remover_arquivo") == 0 || strcmp(cmd, "rm") == 0) {
            if (count >= 2) {
                delete_txt_file(current, arg1);
            } else {
                printf("Uso: remover_arquivo <nome.txt>\n");
            }
        } else {
            printf("Comando não reconhecido: %s\n", cmd);
            printf("Comandos disponíveis: criar_arquivo, criar_pasta, remover_arquivo, remover_pasta, cd, ls, sair\n");
        }
    }
    
    save_filesystem_image(root, "fs.img");
    printf("Sistema de arquivos salvo em fs.img. Encerrando.\n");

    return 0;
}