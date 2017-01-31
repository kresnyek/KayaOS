/* test.e */

extern segTbl_PTR segTable;
extern pteOS_t pageTblOS;

extern pte_t pageTblu2[MAXUPROC];
extern pte_t pagetblu3;

extern uProc_t  procMeta[MAXUPROC];

extern int masterSem;
extern swap_t swamp[NUMOFSWAMP];
int swampMutex;

extern int deviceMutex[7][8];
extern void test();

extern void toggleInterrupts(int flag);
