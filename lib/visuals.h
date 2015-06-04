#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RST "\033[0m"

#define NORMAL 0
#define RED 1
#define GREEN 2
#define WHITE 3
#define RESET -1

/*
printf("%sred\n", KRED);
printf("%sgreen\n", KGRN);
printf("%syellow\n", KYEL);
printf("%sblue\n", KBLU);
printf("%smagenta\n", KMAG);
printf("%scyan\n", KCYN);
printf("%swhite\n", KWHT);
printf("%snormal\n", KNRM);
*/

void consoleColor(int colorCode){
	switch(colorCode){
		case RESET:
			printf("%s", RST);
			break;
		case NORMAL:
			printf("%s", KNRM);
			break;
		case WHITE:
			printf("%s", KWHT);
			break;
		case RED:
			printf("%s", KRED);
			break;
		case GREEN:
			printf("%s", KGRN);
			break;
		default:
			printf("%s", KNRM);
			break;		
	}
}
