#include "types.h"
#include "stat.h"
#include "user.h"

void hello(){
	
	printf(1, "Hello World!!!\n");
	printf(1, "I am Child\n");
	exit();
}

void ct(void){

//		int retval = createThread((uint)hello);

	uint a = (uint)malloc(4096);
	int retval = createThread(a, (uint)hello);

/*	if(retval > 0) {

		printf(1, "In Parent!!!\n");
                printf(1, "My Child: %d\n", retval);
                wait();
                printf(1, "All my children finished their execution\n");


	} else if(!retval){

		hello();
	}
*/
	printf(1, "In Parent!!!\n");
	printf(1, "My Child: %d\n", retval);
	sleep(500);
	wait();
	printf(1, "All my children finished their execution\n");
}

