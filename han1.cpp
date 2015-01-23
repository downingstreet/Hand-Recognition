#include <cv.h>
#include "cvaux.h"
#include "cxmisc.h"
#include "highgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

CvBGCodeBookModel* model = 0;
const int NCHANNELS = 3;
bool ch[NCHANNELS]={true,true,true};

void  detect(IplImage* img_8uc1,IplImage* img_8uc3);

void help(void)
{
    printf("press p for pause";
        );
}


int main(int argc, char** argv)
{
    const char* filename = 0;
    IplImage* rawImage = 0, *yuvImage = 0;
    IplImage *ImaskCodeBook = 0,*ImaskCodeBookCC = 0;
    CvCapture* capture = 0;

    int c, n, nframes = 0;
    int nframesToLearnBG = 300;

    model = cvCreateBGCodeBookModel();


    model->modMin[0] = 3;
    model->modMin[1] = model->modMin[2] = 3;
    model->modMax[0] = 10;
    model->modMax[1] = model->modMax[2] = 10;
    model->cbBounds[0] = model->cbBounds[1] = model->cbBounds[2] = 10;

    bool pause = false;
    bool singlestep = false;

    for( n = 1; n < argc; n++ )
    {
        static const char* nframesOpt = "--nframes=";
        if( strncmp(argv[n], nframesOpt, strlen(nframesOpt))==0 )
        {
            if( sscanf(argv[n] + strlen(nframesOpt), "%d", &nframesToLearnBG) == 0 )
            {
                help();
                return -1;
            }
        }
        else
            filename = argv[n];
    }

    if( !filename )
    {
        printf("Capture from camera\n");
        capture = cvCaptureFromCAM( 0 );
    }
    else
    {
        printf("Capture from file %s\n",filename);
        capture = cvCreateFileCapture( filename );
    }

    if( !capture )
    {
        printf( "Can not initialize video capturing\n\n" );
        help();
        return -1;
    }


    for(;;)
    {
        if( !pause )
        {
            rawImage = cvQueryFrame( capture );
            ++nframes;
            if(!rawImage)
                break;
        }
        if( singlestep )
            pause = true;


        if( nframes == 1 && rawImage )
        {

            yuvImage = cvCloneImage(rawImage);
            ImaskCodeBook = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
            ImaskCodeBookCC = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
            cvSet(ImaskCodeBook,cvScalar(255));

            cvNamedWindow( "Raw", 1 );
            cvNamedWindow( "ForegroundCodeBook",1);
            cvNamedWindow( "CodeBook_ConnectComp",1);
        }

        if( rawImage )
        {
            cvCvtColor( rawImage, yuvImage, CV_BGR2YCrCb );
            if( !pause && nframes-1 < nframesToLearnBG  )
                cvBGCodeBookUpdate( model, yuvImage );

            if( nframes-1 == nframesToLearnBG  )
                cvBGCodeBookClearStale( model, model->t/2 );


            if( nframes-1 >= nframesToLearnBG  )
            {

                cvBGCodeBookDiff( model, yuvImage, ImaskCodeBook );

                cvCopy(ImaskCodeBook,ImaskCodeBookCC);
                cvSegmentFGMask( ImaskCodeBookCC );

                cvShowImage( "CodeBook_ConnectComp",ImaskCodeBookCC);
                detect(ImaskCodeBookCC,rawImage);

            }

            cvShowImage( "Raw", rawImage );
            cvShowImage( "ForegroundCodeBook",ImaskCodeBook);

        }


        c = cvWaitKey(10)&0xFF;
        c = tolower(c);

        if(c == 27 || c == 'q')
            break;

        switch( c )
        {
        case 'h':
            help();
            break;
        case 'p':
            pause = !pause;
            break;
        case 's':
            singlestep = !singlestep;
            pause = false;
            break;
        case 'r':
            pause = false;
            singlestep = false;
            break;
        case ' ':
            cvBGCodeBookClearStale( model, 0 );
            nframes = 0;
            break;

        case 'y': case '0':
        case 'u': case '1':
        case 'v': case '2':
        case 'a': case '3':
        case 'b':
            ch[0] = c == 'y' || c == '0' || c == 'a' || c == '3';
            ch[1] = c == 'u' || c == '1' || c == 'a' || c == '3' || c == 'b';
            ch[2] = c == 'v' || c == '2' || c == 'a' || c == '3' || c == 'b';
            printf("CodeBook YUV Channels active: %d, %d, %d\n", ch[0], ch[1], ch[2] );
            break;
        case 'i':
        case 'o':
        case 'k':
        case 'l':
            {
            uchar* ptr = c == 'i' || c == 'o' ? model->modMax : model->modMin;
            for(n=0; n<NCHANNELS; n++)
            {
                if( ch[n] )
                {
                    int v = ptr[n] + (c == 'i' || c == 'l' ? 1 : -1);
                    ptr[n] = CV_CAST_8U(v);
                }
                printf("%d,", ptr[n]);
            }
            printf(" CodeBook %s Side\n", c == 'i' || c == 'o' ? "High" : "Low" );
            }
            break;
        }
    }

    cvReleaseCapture( &capture );
    cvDestroyWindow( "Raw" );
    cvDestroyWindow( "ForegroundCodeBook");
    cvDestroyWindow( "CodeBook_ConnectComp");
    return 0;
}



void  detect(IplImage* img_8uc1,IplImage* img_8uc3) {


CvMemStorage* storage = cvCreateMemStorage();
CvSeq* first_contour = NULL;
CvSeq* maxitem=NULL;
double area=0,areamax=0;
int maxn=0;
int Nc = cvFindContours(
img_8uc1,
storage,
&first_contour,
sizeof(CvContour),
CV_RETR_LIST
);
int n=0;


if(Nc>0)
{
for( CvSeq* c=first_contour; c!=NULL; c=c->h_next )
{



area=cvContourArea(c,CV_WHOLE_SEQ );

if(area>areamax)
{areamax=area;
maxitem=c;
maxn=n;
}



n++;


}
CvMemStorage* storage3 = cvCreateMemStorage(0);


if(areamax>5000)
{
maxitem = cvApproxPoly( maxitem, sizeof(CvContour), storage3, CV_POLY_APPROX_DP, 10, 1 );

CvPoint pt0;

CvMemStorage* storage1 = cvCreateMemStorage(0);
CvMemStorage* storage2 = cvCreateMemStorage(0);
CvSeq* ptseq = cvCreateSeq( CV_SEQ_KIND_GENERIC|CV_32SC2, sizeof(CvContour),
                                     sizeof(CvPoint), storage1 );
        CvSeq* hull;
        CvSeq* defects;

        for(int i = 0; i < maxitem->total; i++ )
        {   CvPoint* p = CV_GET_SEQ_ELEM( CvPoint, maxitem, i );
            pt0.x = p->x;
            pt0.y = p->y;
            cvSeqPush( ptseq, &pt0 );
        }
        hull = cvConvexHull2( ptseq, 0, CV_CLOCKWISE, 0 );
        int hullcount = hull->total;

        defects= cvConvexityDefects(ptseq,hull,storage2  );





    CvConvexityDefect* defectArray;


       int j=0;
       for(;defects;defects = defects->h_next)
        {
            int nomdef = defects->total;
            if(nomdef == 0)
                continue;


            defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect)*nomdef);


            cvCvtSeqToArray(defects,defectArray, CV_WHOLE_SEQ);

            for(int i=0; i<nomdef; i++)
            {   printf(" defect depth for defect %d %f \n",i,defectArray[i].depth);
                cvLine(img_8uc3, *(defectArray[i].start), *(defectArray[i].depth_point),CV_RGB(255,255,0),1, CV_AA, 0 );
                cvCircle( img_8uc3, *(defectArray[i].depth_point), 5, CV_RGB(0,0,164), 2, 8,0);
                cvCircle( img_8uc3, *(defectArray[i].start), 5, CV_RGB(0,0,164), 2, 8,0);
                cvLine(img_8uc3, *(defectArray[i].depth_point), *(defectArray[i].end),CV_RGB(255,255,0),1, CV_AA, 0 );

            }
            char txt[]="0";
            txt[0]='0'+nomdef-1;
            CvFont font;
            cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1.0, 1.0, 0, 5, CV_AA);
            cvPutText(img_8uc3, txt, cvPoint(50, 50), &font, cvScalar(0, 0, 255, 0));

        j++;


            free(defectArray);
        }


cvReleaseMemStorage( &storage );
cvReleaseMemStorage( &storage1 );
cvReleaseMemStorage( &storage2 );
cvReleaseMemStorage( &storage3 );

}
}
}
