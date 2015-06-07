#define KILO 1024
#define MEGA 1048576

char* concat(char *s1, char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void printSize(long int size){
	if(size < KILO){
		printf("\r%4d bytes transfered.", size);
	}else if(size >= KILO && size < MEGA){
		printf("\r%4d Kbytes transfered.", size/KILO);
	}else if(size >= MEGA){
		printf("\r%4d MBytes transfered.", size/MEGA);
	}
}
