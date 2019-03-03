// Source: http://ch00ftech.com/wp-content/uploads/2012/10/main2.c
/************************************************************
 *Ch00ftech QR code generator
 *This script generates 21x21 Ver. 1 QR codes and can encode 16
 *Characters of text
 ************************************************************/

// Modified not to print or draw anyhting 
// and moved some static data to PROGMEM

//#include <stdio.h>
#include <avr/pgmspace.h>

const unsigned char alpha2int[256] PROGMEM = { \
              1,2,4,8,16,32,64,128,29,58,116,232,205,135,19,38,76,152,45,90,\
							180,117,234,201,143,3,6,12,24,48,96,192,157,39,78,156,37,74,148,\
							53,106,212,181,119,238,193,159,35,70,140,5,10,20,40,80,160,93,186,\
							105,210,185,111,222,161,95,190,97,194,153,47,94,188,101,202,137,15,\
							30,60,120,240,253,231,211,187,107,214,177,127,254,225,223,163,91,\
							182,113,226,217,175,67,134,17,34,68,136,13,26,52,104,208,189,103,\
							206,129,31,62,124,248,237,199,147,59,118,236,197,151,51,102,204,\
							133,23,46,92,184,109,218,169,79,158,33,66,132,21,42,84,168,77,154,\
							41,82,164,85,170,73,146,57,114,228,213,183,115,230,209,191,99,198,\
							145,63,126,252,229,215,179,123,246,241,255,227,219,171,75,150,49,\
							98,196,149,55,110,220,165,87,174,65,130,25,50,100,200,141,7,14,28,\
							56,112,224,221,167,83,166,81,162,89,178,121,242,249,239,195,155,43,\
							86,172,69,138,9,18,36,72,144,61,122,244,245,247,243,251,235,203,\
							139,11,22,44,88,176,125,250,233,207,131,27,54,108,216,173,71,142,1};

const unsigned char int2alpha[256] PROGMEM = { \
              0,0,1,25,2,50,26,198,3,223,51,238,27,104,199,75,4,100,224,14,52,141,\
							239,129,28,193,105,248,200,8,76,113,5,138,101,47,225,36,15,33,53,147,\
							142,218,240,18,130,69,29,181,194,125,106,39,249,185,201,154,9,120,77,\
							228,114,166,6,191,139,98,102,221,48,253,226,152,37,179,16,145,34,136,\
							54,208,148,206,143,150,219,189,241,210,19,92,131,56,70,64,30,66,182,\
							163,195,72,126,110,107,58,40,84,250,133,186,61,202,94,155,159,10,21,\
							121,43,78,212,229,172,115,243,167,87,7,112,192,247,140,128,99,13,103,\
							74,222,237,49,197,254,24,227,165,153,119,38,184,180,124,17,68,146,217,\
							35,32,137,46,55,63,209,91,149,188,207,205,144,135,151,178,220,252,190,97,\
							242,86,211,171,20,42,93,158,132,60,57,83,71,109,65,162,31,45,67,216,183,\
							123,164,118,196,23,73,236,127,12,111,246,108,161,59,82,41,157,85,170,251,\
							96,134,177,187,204,62,90,203,89,95,176,156,169,160,81,11,245,22,235,122,\
							117,44,215,79,174,213,233,230,231,173,232,116,214,244,234,168,80,88,175};

const unsigned char baseoutputmatrix[56] PROGMEM = {
  128,63,192,247,247,137,254,34,209,95,36,250,139,124,127,31,
  160,10,248,255,255,191,255,255,255,255,255,254,255,255,255,
  255,251,255,255,253,63,224,255,247,253,255,162,255,95,244,
  255,139,254,127,223,255,15,248,255,255
};

//returns a single bit value (true or false) from an array which is comprised of a bunch of 8 bit elements
unsigned char getbit(unsigned char * array, int pointer)
{
	if ((array[pointer/8])&(1<<(pointer%8)))
		return 1;
	else
		return 0;
}

//returns score from penalty 1 (consecutive blocks of color)
//this test assigns three points for the first five consecutive blocks of
//color in a set as well as an extra point for every block after that.
unsigned int penalty1(unsigned char * array)
{
	unsigned int scorex=0;
	unsigned int scorey=0;
	unsigned int i;
	unsigned int j;
	
	//keep track of the current color in the "run"
	char currentcolorx =0;
	char currentcolory=0;
	
	//keep track of current number of elements in a row
	char currentcountx = 0;
	char currentcounty = 0;
	
	
	
	for (i=0;i<21;i++)
	{
		for (j=0;j<21;j++)
		{
			//horizontal check
            //if the current pixel is the same color as the last one
			if (((getbit(&array[0], ((21*i)+j))==0) && (currentcolorx==0))||((getbit(&array[0], ((21*i)+j))!=0) && (currentcolorx!=0)))
			{
                //if this is the fifth same color in a row, add 3 to the score
				currentcountx++;
				if (currentcountx==5)
				{
					scorex+=3;
				}
                 //for every one after 5, add one to the score
				if (currentcountx>5)
				{
					scorex+=1;
				}
				
			}
			else
			{
                //if the new color is different, toggle "currentcolor"
				currentcountx=1;
				if (currentcolorx)
					currentcolorx=0;
				else
					currentcolorx=1;
			}
            //vertical check
			//if the current pixel is the same color as the last one
			if (((getbit(&array[0], ((21*j)+i))==0) && (currentcolory==0))||((getbit(&array[0], ((21*j)+i))!=0) && (currentcolory!=0)))
			{
                //if this is the fifth same color in a row, add 3 to the score
				currentcounty++;
				if (currentcounty==5)
				{
					scorey+=3;
				}
                //for every one after 5, add one to the score
				if (currentcounty>5)
				{
					scorey+=1;
				}
			}
			else
			{
                //if the new color is different, toggle "currentcolor"
				currentcounty=1;
				if (currentcolory)
					currentcolory=0;
				else
					currentcolory=1;
			}
		}
		currentcountx=0;
		currentcounty=0;
	}
	return scorey+scorex;
}

//returns score from penalty 2 (2x2 blocks of same color)
//Every time a 2x2 block appears of the same color, this test assigns three penalty points
unsigned int penalty2(unsigned char * array)
{
	unsigned int i;
	unsigned int j;
	unsigned int score=0;
	for (i=0;i<20;i++)
	{
		for (j=0;j<20;j++)
		{
			//printf("%s","X: ");
			//printf("%d",j);
			//printf("%s"," Y: ");
			//printf("%d",i);
			if ((getbit(&array[0],((21*i)+j)))&&(getbit(&array[0],((21*(i+1))+j)))&&(getbit(&array[0],((21*i)+j+1)))&&(getbit(&array[0],((21*(i+1))+j+1))))
			{
				score+=3;
			//	printf("%s","BING");
			}
			//printf("%s","\n");
			if (!(getbit(&array[0],((21*i)+j)))&&!(getbit(&array[0],((21*(i+1))+j)))&&!(getbit(&array[0],((21*i)+j+1)))&&!(getbit(&array[0],((21*(i+1))+j+1))))
				score+=3;
		}
	}
	return score;
}

//returns score from penalty 3 (blocks that look like positioning boxes)
unsigned int penalty3(unsigned char * array)
{
	unsigned int i;
	unsigned int j;
	unsigned int score=0;
	
	for(i=0;i<21;i++)
	{
		for (j=0;j<21;j++)
		{
			char k;
			char passx = 1;
			char passy = 1;
			
			//checking for "â–ˆ â–ˆâ–ˆâ–ˆ â–ˆ    "
			unsigned int pattern = 559;
			for (k=0;k<11;k++)
			{
                if (j<12)
				{
					if (!(getbit(&array[0],(21*(i))+j+k) == ((pattern>>k)&1)))
						passx=0;
				}
				else
					passx = 0;
				if (i<12)
				{
					if (!(getbit(&array[0],(21*(i+k))+j) == ((pattern>>k)&1)))
						passy=0;
				}
				else
					passy=0;
			}
			score+=((40*passx)+(40*passy));
			
			//checking for "    â–ˆ â–ˆâ–ˆâ–ˆ â–ˆ"
			passx=1;
			passy=1;
			pattern = 1954;
			for (k=0;k<11;k++)
			{
				if (j<12)
				{
					if (!(getbit(&array[0],(21*(i))+j+k) == ((pattern>>k)&1)))
						passx=0;
				}
				else
					passx=0;
				if(i<12)
				{
					if (!(getbit(&array[0],(21*(i+k))+j) == ((pattern>>k)&1)))
						passy=0;
				}
				else
					passy=0;
			}
			score+=((40*passx)+(40*passy));
			
			//checking for "    â–ˆ â–ˆâ–ˆâ–ˆ â–ˆ    "
			passx=1;
			passy=1;
			pattern = 31279;
			for (k=0;k<15;k++)
			{
				if (j<12)
				{
					if (!(getbit(&array[0],(21*(i))+j+k) == ((pattern>>k)&1)))
						passx=0;
				}
				else
					passx=0;
				if (i<12)
				{
					if (!(getbit(&array[0],(21*(i+k))+j) == ((pattern>>k)&1)))
						passy=0;
				}
				else
					passy=0;
			}
			score+=((40*passx)+(40*passy));
		}
	}
	return score;
}
	

//calculates the penalty which has to do with how balanced the white and dark pixels are.
unsigned int penalty4(unsigned char * array)
{
	unsigned int blackcount=0;
	unsigned int score=0;
	char i;
	char j;
	
	for (i=0;i<21;i++)
	{
		for (j=0;j<21;j++)
		{
			if (!getbit(&array[0],(i*21)+j))
				blackcount++;
		}
	}
	score = ((blackcount*100)/441);
	//a "perfect score" is 50.  This part calculates how far off you are from 50.
	if (score>=50)
		score-=50;
	else if (score<50)
		score=49-score; //the 49 is to simulate the truncation that would have happened if I was using floating point.
	score*=2;
	return score;
}

/*

//"prints" QR code
void printcode(unsigned char * array)
{
	int i = 0;
	int j = 0;
	for (i=0; i<21; i++)
	{
		for (j=0; j<21; j++)
		{
			if (getbit(&array[0],(21*i)+j))
				//printf("%s"," ");
			else
				//printf("%s","â–ˆ");
		}
		//printf("%d",i);
		//printf("%s","-\n");
	}
}
*/			

unsigned int asciiconvert(char asciichar)
{
	if (asciichar >=48 && asciichar <=57) //character is a number
		return asciichar-48;
	else if(asciichar >=65 && asciichar<=90) //character is a letter
		return asciichar-55;
	else if(asciichar == 32) //space
		return 36;
	else if(asciichar == 36)//$
		return 37;
	else if(asciichar == 37)//%
		return 38;
	else if(asciichar == 42)//*
		return 39;
	else if(asciichar == 43)//+
		return 40;
	else if(asciichar == 45)//-
		return 41;
	else if(asciichar == 46)//.
		return 42;
	else if(asciichar == 47)// /
		return 43;
	else if(asciichar == 58)//:
		return 44;
	else return 36; //return a space by default.
}
	

//returns the location of the first nonzero element in the array (assuming array 13 elements long)
unsigned char firstnonzero(unsigned char array[])
{
	unsigned char i=13;
	
	while (array[i]==0 && i>0)
	{
		i--;
	}
	return i;
}

//this function adds a bit to an array of chars working left-justified.
//it adds the bit at the location specified by the "pointer" pointer
void addbit(unsigned char * array, int * pointer, char bit)
{
	char bytenum = *pointer/8;
	char bitnum = *pointer%8;
	if (bit==1)
	{
		array[bytenum] = array[bytenum]|(1<<bitnum);
	}
	else
	{
		array[bytenum] = array[bytenum]&(~(1<<bitnum));
	}
}

//this function adds multiple bits to an array of chars.
//it works just like addbit, but it will add "length" many bits.
//note, if length isn't set right, it could truncate your bits.
void addbits(unsigned char * array, int * pointer, unsigned int bits, char length)
{
	length--;
	char i;
	for (i=length;i>=0;i--)
	{
		addbit(array,pointer,(((1<<i)&bits)>>i));
		*pointer=*pointer-1;
	}
}



//this function adds multiple bits to an array of chars.
//it works just like addbit, but it will add "length" many bits.
//note, if length isn't set right, it could truncate your bits.
void addbitsrightjustified(unsigned char * array, int * pointer, unsigned int bits, char length)
{
	char i;
	for (i=0;i<length;i++)
	{
		addbit(array,pointer,(((1<<i)&bits)>>i));
		*pointer=*pointer+1;
	}
}

//inserts a bit into the appropriate location in the array
//so that it will show up in the provided x and y coordinates
void addmatrixbit(unsigned char *array, char x, char y, char bit)
{
	int temp = (y*21)+x;
	addbit(array, &temp, bit);
}

//swaps the value of the bit in the x/y location
void swapmatrixbit(unsigned char *array, char x, char y)
{
	int pointer = (y*21)+x;
	if (getbit(&array[0], pointer))
		addbit(&array[0], &pointer,0);
	else
		addbit(&array[0], &pointer,1);
}
	
//adds timing pattern and that one random black pixel
void addtimingpatternandblackpixel(unsigned char * array)
{
	char i;
	for (i=0;i<3;i++)
	{
		addmatrixbit(&array[0], 6, (2*i)+8,0);
		addmatrixbit(&array[0],(2*i)+8,6,0);
	}
	addmatrixbit(&array[0],8,13,0);
}


//this function will add the position detection markers to the array.
//starting with the location specified as the top left corner of the marker
//this function is dumb, so make sure the input locations are valid.
void addposdetectmarkers(unsigned char * array, char x, char y)
{
	char i;
	
	for (i=0;i<7;i++)
	{
		addmatrixbit(&array[0],x+i,y,0);
		addmatrixbit(&array[0],x+i,y+6,0);
	}
	
	for (i=0;i<5;i++)
	{
		addmatrixbit(&array[0],x,y+1+i,0);
		addmatrixbit(&array[0],x+6,y+1+i,0);
	}
	
	for (i=0;i<3;i++)
	{
		addmatrixbit(&array[0],x+2,y+2+i,0);
		addmatrixbit(&array[0],x+3,y+2+i,0);
		addmatrixbit(&array[0],x+4,y+2+i,0);
	}
}		

//this function takes the message array and inserts it into the matrix
//in the correct order.
//note that 1s are black and 0s are white.
//also, the "first bit" (starts in bottom right) is actually the highest
//bit of the array
void adddatabits(unsigned char * matrix, unsigned char * data)
{
	char i;
	//first column up
	int pointer=207;
	
	for (i=0;i<12;i++)
	{
        
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],20,20-i,0);
		pointer--;
        
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],19,20-i,0);
		pointer--;
	}
	
	//second column down
	for (i=0;i<12;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],18,9+i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],17,9+i,0);
		pointer--;
	}
	
	//third column up
	for (i=0;i<12;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],16,20-i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],15,20-i,0);
		pointer--;
	}
	
	//fourth column down
	for (i=0;i<12;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],14,9+i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],13,9+i,0);
		pointer--;
	}
	
	//fifth column up (skipping horizontal timing band)
	for (i=0;i<14;i++)
	{
		
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],12,20-i,0);
		pointer--;
		
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],11,20-i,0);
		pointer--;
	}
	
	for (i=0;i<6;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],12,5-i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],11,5-i,0);
		pointer--;
	}
	
	//sixth column down (skipping horizontal timing band)
	for (i=0;i<6;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],10,i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],9,i,0);
		pointer--;
	}
	
	for (i=0;i<14;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],10,7+i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],9,7+i,0);
		pointer--;
	}
	
	//seventh column up
	for (i=0;i<4;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],8,12-i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],7,12-i,0);
		pointer--;
	}	
	
	//eigth column down (skipping vertical timing band)
	for (i=0;i<4;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],5,9+i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],4,9+i,0);
		pointer--;
	}
	
	//ninth column up
	for (i=0;i<4;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],3,12-i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],2,12-i,0);
		pointer--;
	}	
	
	//tenth column down
	for (i=0;i<4;i++)
	{
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],1,9+i,0);
		pointer--;
		if (getbit(&data[0],pointer))
			addmatrixbit(&matrix[0],0,9+i,0);
		pointer--;
	}
	 

}


//This function adds the error correction and bitmask type information to the QR code
void addtypeinformation(unsigned char * array, char type)
{
	unsigned int info; 
	
	char i;
	
	switch (type)
	{
		case 0:
			info=13663;
			break;
		case 1:
			info=12392;
			break;
		case 2:
			info=16177;
			break;
		case 3:
			info=14854;
			break;
		case 4:
			info=9396;
			break;
		case 5:
			info=8579;
			break;
		case 6:
			info=11994;
			break;
		case 7:
			info=11245;
			break;
	}
		
	for (i=0;i<6;i++)
	{
		if (info&(1<<i))
		{
			addmatrixbit(&array[0],8,i,0);
			addmatrixbit(&array[0],20-i,8,0);
		}
	}
	if (info&1<<6)
	{
		addmatrixbit(&array[0],14,8,0);
		addmatrixbit(&array[0],8,7,0);
	}
	if (info&1<<7)
	{
		addmatrixbit(&array[0],13,8,0);
		addmatrixbit(&array[0],8,8,0);
	}
	if (info&1<<8)
	{
		addmatrixbit(&array[0],8,14,0);
		addmatrixbit(&array[0],7,8,0);
	}
	for (i=0;i<6;i++)
	{
		if (info&(1<<(i+9)))
		{
			addmatrixbit(&array[0],5-i,8,0);
			addmatrixbit(&array[0],8,15+i,0);
		}
	}
		
	
}
	
char maskindex(unsigned int x, unsigned int y, char masktype)
{
	switch (masktype)
	{
		case 0:
			return (x+y)%2==0;
			break;
		case 1:
			return y%2==0;
			break;
		case 2:
			return x%3==0;
			break;
		case 3:
			return (x+y)%3==0;
			break;
		case 4:
			return ((y/2)+(x/3))%2==0;
			break;
		case 5:
			return ((y*x)%2)+((y*x)%3)==0;
			break;
		case 6:
			return (((y*x)%2)+((y*x)%3))%2==0;
			break;
		case 7:
			return (((y+x)%2)+((y*x)%3))%2==0;
			break;
		default:
			return 0;
	}
}
			

void applymask(unsigned char * array, char masktype)
{
	unsigned int i;
	unsigned int j;
	for (i=9;i<21;i++)
	{
		for (j=9;j<21;j++)
		{
			if (maskindex(i,j,masktype))
				swapmatrixbit(&array[0],i,j);
		}
	}
	
	for (i=9;i<13;i++)
	{
		for (j=0;j<6;j++)
		{
			if (maskindex(i,j,masktype))
				swapmatrixbit(&array[0],i,j);
			if (maskindex(j,i,masktype))
				swapmatrixbit(&array[0],j,i);
		}

	}

	for (i=9;i<13;i++)
	{
		for (j=7;j<9;j++)
		{
			if (maskindex(i,j,masktype))
				swapmatrixbit(&array[0],i,j);
			if (maskindex(j,i,masktype))
				swapmatrixbit(&array[0],j,i);
		}
	}
	

}

void generate_qr_code (const char *input, unsigned char *outputmatrix) {

/*****************************************
*  Generate Message
*  
*  This section generates the actual byte
*  code that will be visually represented
*  by the QR.
******************************************/
	
	
	///////////////////////
	//Variable declarations
	///////////////////////
	
    
	//raw input string from user
	//char input[17];
	
	//number of characters in raw input
	unsigned char inputlength = 0;
	
	//array of bits generated from original raw input
	unsigned char binarray[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	//This pointer will tell me where to add bits in that array
	//This is necessary, because we will be dealing with individual bits, not bytes.
	int binarraypointer = 103;
	
	//final output array from this whole process
	unsigned char outputarray[26]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	//dummy for loop counter
	unsigned char i;
	
  /*	
	///////////////////////
	//Grab user input
	///////////////////////
	printf("input string (terminate with semicolon)\n");
	fgets(input,18,stdin);
	printf("%s",input); //Read back the input so the user will know if it got truncated
	printf("%s","\n");
  */
	
	for (i=0; i<18; i++)
	{
		if (input[i]==59) //seek out semicolon
			inputlength=i;
	}
	
	////////////////////////////////////////////////
	//Add QR header
    //
    //The QR header encodes important information
    //Such as the data encoding mode and QR version
	////////////////////////////////////////////////
	
	//operating in Alphanumeric Mode (mode 2)
	addbits(&binarray[0], &binarraypointer, 2,4);
 
    //in version 1, so we need nine bits to encode the message length
	addbits(&binarray[0], &binarraypointer, inputlength,9);
	

    
	///////////////////////
	//Add message data
	///////////////////////
	
	//add the first even number of characters by converting pairs to 11 bit values using the special lookup
	for (i=0; i<(inputlength/2);i++)
	{
        addbits(&binarray[0], &binarraypointer, asciiconvert(input[2*i])*45+asciiconvert(input[2*i+1]),11);
	}
	//if there's an odd number of characters, add the final six bits for the odd character out
	if (inputlength%2)
		addbits(&binarray[0],&binarraypointer,asciiconvert(input[inputlength-1]),6);
	
	//add extra zeroes
	
    

    
	//fit up to four zeroes after the data space permitting
	char numzeroestopad = (char)binarraypointer;
	if (numzeroestopad >4)
		numzeroestopad=4;
	addbits(&binarray[0],&binarraypointer,0x000,numzeroestopad); //pad with zeroes as appropriate
	
	
	//round data length off to the nearest byte, space permitting
	addbits(&binarray[0],&binarraypointer,0x000,(binarraypointer+1)%8);
	
    
        
	
	//If the extra zeroes aren't enough to fill out the 104 bit limit, add dummy bytes as appropriate

	//this char keeps track of which of the two types of extra byte was just added
	char extrabytenum = 0; 
	
	while (binarraypointer>0)
	{
		if (extrabytenum)
		{
			addbits(&binarray[0],&binarraypointer,0x11,8);
			extrabytenum = 0;
		}
		else
		{
			addbits(&binarray[0],&binarraypointer,0xEC,8);
			extrabytenum=1;
		}
	}

	//pump message data out to output string before creating codewords
	for (i=0; i<13; i++)
	{
		outputarray[i+13]=binarray[i];
	}
    
   
	////////////////////////////////////////////////////
	//create codewords
    //
    //Codewords are the QR message with all the error
    //correcting magic done
	///////////////////////////////////////////////////
	
	
	//Generator polynomial for degree 13
	//These are the exponents to the polynomial in alpha form
    //These numbers were generated automagically with an online tool
    //In reality, it's a pretty intense process
	const unsigned char genpolyalphas[14] = {78,140,206,218,130,104,106,100,86,100,176,152,74,0};
	
	
	//left justify binaryarray so that its first term lines up with the first polynomial term
    //When i drops below zero, it will roll over to being larger than 14
	for (i=13;i<14;i--)
	{
		binarray[i]=binarray[i-1];
	}
	
	
	//add a zero in the space that was just cleared up
	binarray[0]=0;
	
    	//variable that keeps track of string after the multiplication step
	unsigned char temp[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	
	//binarray is going to now be used to store the polynomial before multiplication step
	//these two arrays are XORed together at the end of the step.

    

    
	char j;
    j=0;
	while (j<13) //keep iterating until the final term of the polynomial has x^0
	{
        
      
        
        //multiply generator polynomial in alpha form by first coefficient of previous polynomial in alpha form.
        //if you're just starting out, the "previous polynomial" is the message polynomial
        //don't forget the mod 255 NOT MOD 256
        for (i=0;i<14;i++)
        {
            //you're basically just adding exponents
            //unsigned int overflow = genpolyalphas[i]+int2alpha[binarray[13]];
            unsigned int overflow = genpolyalphas[i]+pgm_read_byte(int2alpha + binarray[13]);
            temp[i]=(overflow)%255;
        }
       
           
        //Perform the XOR step making sure to XOR the integer coefficients of the modified generator polynomial
        //with the previous polynomial (if you're just starting out, it's the message polynomial)
        for (i=0;i<14;i++)
        {
        //temp[i] = alpha2int[temp[i]]^binarray[i];
        temp[i] = pgm_read_byte(alpha2int + temp[i])^binarray[i];
        }


        //store array in binarray for future use (and left shift it to get rid of leading zeroes
        //UPDATE 4/2/2015: Turns out you only need to get rid of first leading zero.
        unsigned char topnonzero = 12;//firstnonzero(temp);

        /*
        printf("%d,",topnonzero);
        printf("\n");
        */
        for (i=0; i<(13-topnonzero);i++) //add trailing zeroes too.
            binarray[i]=0;
        
        
        for (i=0; i<topnonzero+1; i++)
            binarray[(13-topnonzero)+i]=temp[i];
        
        j+=(13-topnonzero);
	}
    
	
	//Add the codewords to the output array
	for (i=0;i<13;i++)
	{
		outputarray[i]=temp[i];
	}
  
    

    
/*****************************************
 *  BeginLayout
 ******************************************/	
/*
	unsigned int bestscore=5000; //let's hope no scores are higher than that.
	unsigned char bestmask=0;
	for (i=0;i<8;i++)
	{
	//output matrix.
	//output runs from top left to bottom right with 7 bits left over
	unsigned char outputmatrix[56]={0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF,0XFF};

	//Add position detection markers and timing pattern
	addposdetectmarkers(&outputmatrix[0],0,0);
	addposdetectmarkers(&outputmatrix[0],14,0);
	addposdetectmarkers(&outputmatrix[0],0,14);
	addtimingpatternandblackpixel(&outputmatrix[0]);
	
	
	adddatabits(&outputmatrix[0],&outputarray[0]);
	addtypeinformation(&outputmatrix[0],i);
	applymask(&outputmatrix[0],i);
	
	printcode(&outputmatrix[0]);

		printf("%s","penalty1: ");
		printf("%d",penalty1(&outputmatrix[0]));
		printf("%s","\n");
		printf("%s","penalty2: ");
		printf("%d",penalty2(&outputmatrix[0]));
		printf("%s","\n");
		printf("%s","penalty3: ");
		printf("%d",penalty3(&outputmatrix[0]));
		printf("%s","\n");
		printf("%s","penalty4: ");
		printf("%d",penalty4(&outputmatrix[0]));
		printf("%s","\n");

		
		unsigned int currentscore =(penalty1(&outputmatrix[0])+penalty2(&outputmatrix[0])+penalty3(&outputmatrix[0])+penalty4(&outputmatrix[0]));
		
		if (currentscore<bestscore)
		{
			bestscore=currentscore;
			bestmask=i;
		}
		
	}
*/	
    //just for fun, I took out the "add markers" and stuff here and just incorporated them into the starting array
  // Moved base data to top 

  // Copy to baseoutputmatrix to outputmatrix
  memcpy_P(outputmatrix, &baseoutputmatrix, sizeof(baseoutputmatrix));

  
	adddatabits(outputmatrix,&outputarray[0]);
	addtypeinformation(outputmatrix,0);
	applymask(outputmatrix,0);

/*
   	printcode(&outputmatrix[0]);
    printf("%d",bestscore);

  return 0;
*/
	
}
