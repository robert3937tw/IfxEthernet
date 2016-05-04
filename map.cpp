///////////////////////////////////////
//       Map Building algorithm      //
//       sensor:TOF                  //
//       plateform:IMX6              //
//   	 communication:Aurix         //
//		 ver:2.0                     //
///////////////////////////////////////
#include"TCPclient.h"
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<time.h>

///////////////////////////////////
//          definition           //
///////////////////////////////////
#define bigside 121
#define occ 255                             //This cell is occupied.
#define threshold 150                         //The cell is occupied when the number of points in a cell are over treshold. (�i��)
#define width 224                            //width is equal to the width of tof view.


///////////////////////////////////
//        function declare       //
///////////////////////////////////

void tof2D(float*, float*, float*);                                            //D=(x^2+y^2)^(1/2):the distance between tof and the detected point
void tof2sita(float*, float*, float*, float*);                                         //sita=asin(x/D)
float *pointrelativex(float x1[], float D[], float sita[], float, int);                           //the relative distance between tof and the detected point in x
float *pointrelativey(float y1[], float D[], float sita[], float, int);                             //the relative distance between tof and the detected point in y
void gridize(float*, float*, int*, int*);
void updatecell(int*, int*, unsigned char globalmap[], float*, int, int);            //This function is to decide whether a cell is occupied or not.

///////////////////////////////////
//         main function         //
///////////////////////////////////

int main(void)
{
	//input from TOF//	
	float x[width];              //different x in certain y
	float z[width];              //different depth in each x in certain y
	float y[width];
	float confidence[width];

	//input from Aurix//
	int xr, yr;                  //xr,yr:Which cell robot is in a grid.
	float robotviewsita;         //the angle between tof view and y axis

	//temp//
	//int mx[width] = {0};
	//int my[width] = {0};

	//output to Aurix//
	unsigned char globalmap[14641]={0};              //the whole map is now.

	//main func. variables//
	float D[width]={0};
	float sita[width]={0};
	float *ptrx;                 //the ouput of pointrelativex function
	float *ptry;                 //the ouput of pointrelativey function
	int pointx[width]={0};                 //the point which has been through gridize func.,defined which grid it belongs to.
	int pointy[width]={0};                 //the point which has been through gridize func.,defined which grid it belongs to.
	int i,k;
    FILE *fptr;
    FILE *fptro;
	float x1[width];
	float y1[width];
	//int temp;
	//initial//
	xr=0;
	yr=0;
	robotviewsita=0;
	
	//get data//
	
	fptr=fopen("../frame.txt","r");
	
	for(i=0;i<width;i++)
		fscanf(fptr,"%f %f %f %f",&x[i],&y[i],&z[i],&confidence[i]);
	printf("Read data completed\n");
	fclose(fptr);
	
	/*
	for(i=0;i<223;i++)
	{
		printf("%f ",y[i]);
		printf("%f ",x[i]);
		printf("%f ",z[i]);
		printf("%f ",confidence[i]);
		printf("\n");
    }
	*/	
	
	//grid-cell update//
	tof2D(x, z, D);
    printf("Calaulate distance...\n");
	tof2sita(x, D, sita, confidence);  
    printf("Calaulate angle...\n");
	/*
	for(i=0;i<223;i++)
	{
		printf("%f %f\n",D[i],sita[i]);
	}*/
	ptrx = pointrelativex(x1, D, sita, robotviewsita, xr);
	ptry = pointrelativey(y1, D, sita, robotviewsita, yr);
	printf("Transfer Coordinate...\n");
	/*
	for(i=0;i<223;i++)
	{
		printf("%f %f\n",*(ptrx+i),*(ptry+i));
	}*/
	
	gridize(ptrx, ptry, pointx, pointy);

	
	for(i=0;i<223;i++)
	{
		printf("%d %d\n",pointx[i],pointy[i]);
	}
	
	printf("Map update...\n");
	updatecell(pointx, pointy, globalmap, confidence, xr, yr); 
    printf("DONE\n");
	
	//print map//
	/*
	for(i=0;i<bigside;i++)
	{
		for(k=0;k<bigside;k++)
	        printf("%d",globalmap[bigside*i+k]);
	    
		    
	    printf("%d\n",i);
	}
	*/
	
	//test write//
	fptro=fopen("../globalmap.txt","w");
	
	for(i=0;i<14641;i++)
	{
		//temp=(int)globalmap[i];
		fprintf(fptro,"%d ",globalmap[i]);
	}
	
	
	fclose(fptro);
	
	system("pause");
	return 0;
}


/////////////////////////////////////////
//        function identification      //
/////////////////////////////////////////

void tof2D(float* x, float* z, float* D)                                  //��D if depth_data���s�Ȭ�D����ƧY���� 
{
	int i;

	for (i = 0; i < width; i++)
		*(D+i) = sqrt((*(x + i ))*(*(x + i )) + (*(z + i ))*(*(z + i )));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void tof2sita(float* x, float* D, float* sita, float* confidence)                 //��sita 
{
	int i;

	for (i = 0; i < width; i++)
	{
		if(*(confidence+i)==255)
		    *(sita+i) = asin((*(x + i )) / (*(D+i)));
	}
		
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

float *pointrelativex(float* x1, float* D, float* sita, float robotviewsita, int xr)          //��xy,x��cm���i,y��D,sita��float !!p1,p2�i�H�bmain�̥�ptr�����A�~���|���к�D��sita
{
	int i;
	
	for (i = 0; i < width; i++)
	{

		*(x1+i) = 0.1*xr + (*(D+i))*sin(robotviewsita + *(sita+i));
	}

	return x1;
}

float *pointrelativey(float* y1, float* D, float* sita, float robotviewsita, int yr)          //��xy,x��cm���i,y��D,sita��float 
{
	int i;

	for (i = 0; i < width; i++)
	{
		*(y1+i) = 0.1*yr + (*(D+i))*cos(robotviewsita + *(sita+i));
	}

	return y1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void gridize(float* ptrx,float* ptry,int* pointx,int* pointy)
{
	int i;
	
	for(i=0;i<width;i++)
	{
		*(pointx+i)=(int)roundf(*(ptrx+i)/0.1);
		*(pointy+i)=(int)roundf(*(ptry+i)/0.1);
	}
}

//��aurix(global��}�C)
void updatecell(int* pointx,int* pointy,unsigned char globalmap[], float* confidence,int xr, int yr) 
{
	int i,j,k,checkx,checky,valid=0;
	unsigned char tempmap[14641]={0};
	//int x[width]={0};
	//int y[width]={0};
    
	//occupied management
	for(i=0;i<width;i++)
	{
		if((*(confidence+i)==255) && (tempmap[bigside*(*(pointy+i)+60)+*(pointx+i)+60]<=250))
		{
			tempmap[bigside*(*(pointy+i)+60)+*(pointx+i)+60]=/*tempmap[bigside*(*(pointy+i)+60)+*(pointx+i)+60]+*/255;
		/*	if(tempmap[bigside*(*(pointy+i)+60)+*(pointx+i)+60]==255)
			{
				x[valid]=*(pointx+i);
				y[valid]=*(pointy+i);
				valid++;
			}*/
		}
			
	}
	/*
    printf("%d\n",valid);
    for(i=0;i<valid;i++)
    {
    	printf("%d %d\n",x[i],y[i]);
    }
    *//*
	for(i=0;i<width;i++)
	{
		if(tempmap[bigside*(*(pointy+i)+60)+*(pointx+i)+60]<threshold)
		{
			tempmap[bigside*(*(pointy+i)+60)+*(pointx+i)+60]=0;
		}
	}
	
	//decrease
	
	for(i=0;i<valid;i++)  
	{
		//printf("1\n");
		if(xr+1<=x[i] && yr<=y[i]) //xy�k�W
		{
			//printf("1\n");
            for(j=xr+1;j<=x[i]-1;j++)
			{
				checky=roundf(((y[i]-yr)/(x[i]-xr))*(j-xr)+yr);
				if(globalmap[bigside*checky+j]!=0)
					globalmap[bigside*checky+j]=0;
			}
			
			for(k=yr+1;k<=y[i]-1;k++)
			{
				checkx=roundf(((x[i]-xr)/(y[i]-yr))*(k-yr)+xr);
				if(globalmap[bigside*k+checkx]!=0)
					globalmap[bigside*k+checkx]=0;
			}
			
		}
		
		else if(xr<=x[i] && yr>y[i]) //xy�k�U
		{
			//printf("2\n");
            for(j=xr+1;j<=x[i]-1;j++)
			{
				checky=roundf(((y[i]-yr)/(x[i]-xr))*(j-xr)+yr);
				if(globalmap[bigside*checky+j]!=0)
					globalmap[bigside*checky+j]=0;
			}
			
			for(k=yr-1;k>=y[i]+1;k--)
			{
				checkx=roundf(((x[i]-xr)/(y[i]-yr))*(k-yr)+xr);
				if(globalmap[bigside*k+checkx]!=0)
					globalmap[bigside*k+checkx]=0;
			}
		}
		else if(xr+1>=x[i] && yr>=y[i]) //���U
		{
			//printf("3\n");
            for(j=xr-1;j>=x[i]+1;j--)
			{
				checky=roundf(((y[i]-yr)/(x[i]-xr))*(j-xr)+yr);
				if(globalmap[bigside*checky+j]!=0)
					globalmap[bigside*checky+j]=0;
			}
			
			for(k=yr-1;k>=y[i]+1;k--)
			{
				checkx=roundf(((x[i]-xr)/(y[i]-yr))*(k-yr)+xr);
				if(globalmap[bigside*k+checkx]!=0)
					globalmap[bigside*k+checkx]=0;
			}
		}
		else if(xr>=x[i] && yr+1<=y[i]) //���W
		{
			//printf("4\n");
            for(j=xr-1;j>=x[i]+1;j--)
			{
				checky=roundf(((y[i]-yr)/(x[i]-xr))*(j-xr)+yr);
				if(globalmap[bigside*checky+j]!=0)
					globalmap[bigside*checky+j]=0;
			}
			
			for(k=yr+1;k<=y[i]-1;k++)
			{
				checkx=roundf(((x[i]-xr)/(y[i]-yr))*(k-yr)+xr);
				if(globalmap[bigside*k+checkx]!=0)
					globalmap[bigside*k+checkx]=0;
			}
		}
	}
	*/
	//map build
	for(i=0;i<14641;i++)
		if(globalmap[i]==0)
		globalmap[i]=globalmap[i]+tempmap[i];
	
	
}




