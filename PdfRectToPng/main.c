//
//  main.c
//  PdfRectToPng
//
//  Created by Brett Hutley on 03/10/2011.
//  Copyright 2011 Stimuli Limited. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ApplicationServices/ApplicationServices.h>

static CGPDFDocumentRef getPDFDocumentRef(CFStringRef path) 
{
    CFURLRef url = CFURLCreateWithFileSystemPath (NULL, path, kCFURLPOSIXPathStyle, 0);
    CGPDFDocumentRef document = CGPDFDocumentCreateWithURL (url);// 2
    CFRelease(url);
    size_t count = CGPDFDocumentGetNumberOfPages (document);// 3
    if (count == 0) {
        //NSLog([NSString stringWithFormat:@"'%@' needs at least one page!", path]);
        return NULL;
    }
    return document;
}

CGRect displayPDFPage(CGContextRef myContext,size_t pageNumber, CFStringRef filename, int imgHeight) 
{
    CGPDFPageRef page;
    CGPDFDocumentRef document = getPDFDocumentRef (filename);// 1
    //int totalPages=CGPDFDocumentGetNumberOfPages(document);
    page = CGPDFDocumentGetPage (document, pageNumber);// 2
    CGRect rect = CGPDFPageGetBoxRect(page, kCGPDFCropBox);
    //CGContextRotateCTM(myContext, M_PI);
    //CGContextTranslateCTM(myContext, -300, -300);
    CGContextTranslateCTM(myContext, 0, imgHeight);
    CGContextScaleCTM(myContext, 1, -1);
    CGContextClipToRect(myContext, rect);
    CGContextDrawPDFPage (myContext, page);// 3
    //CGContextTranslateCTM(myContext, 0, 20);
    //CGContextScaleCTM(myContext, 1.0, -1.0);
    CGPDFDocumentRelease (document);// 4
    return CGContextConvertRectToUserSpace(myContext, rect);
}

int main (int argc, const char * argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <file.pdf>\n", argv[0]);
        exit(0);
    }
    
    const char *pdfFilename = argv[1];
    if (strcasecmp(pdfFilename + strlen(pdfFilename) - 4, ".pdf"))
    {
        printf("Specified file must be a PDF file\n");
        exit(0);
    }
    
    if (strlen(pdfFilename) > 4095)
    {
        printf("Specified file path is too long!\n");
        exit(0);
    }
    
    struct stat st;
    if (stat(pdfFilename, &st) != 0) 
    {
        printf("There was a problem reading file '%s'\n", pdfFilename);
        exit(0);
    }

    char outFilename[4096];
    strcpy(outFilename, pdfFilename);
    strcpy(outFilename + strlen(outFilename) - 3, "png");

    CFStringRef path = CFStringCreateWithCString(NULL, pdfFilename, CFStringGetSystemEncoding());
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, 0);
    CGPDFDocumentRef document = CGPDFDocumentCreateWithURL(url);
    CFRelease(url);
    CFRelease(path);

    CGPDFPageRef page = CGPDFDocumentGetPage(document, 1);
    CGRect selRect = CGPDFPageGetBoxRect(page, kCGPDFCropBox);
    CGRect pageRect = CGPDFPageGetBoxRect(page, kCGPDFMediaBox);


    int width = selRect.size.width;
    int height = selRect.size.height;
    int bytesPerPixel = 4;
    int bytesPerRow = bytesPerPixel * width;
    int bitsPerComponent = 8;
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceCMYK();    
    
    void *data = malloc(bytesPerPixel * height * width);
    CGContextRef context = CGBitmapContextCreate(
                                        data,
                                        width,
                                        height,
                                        bitsPerComponent,
                                        bytesPerRow,
                                        colorSpace,
                                        kCGImageAlphaNone
                                        );

    CGContextTranslateCTM(context, -selRect.origin.x, -selRect.origin.y);
    //CGContextScaleCTM(context, 1, -1);
    CGContextClipToRect(context, selRect);
    CGContextDrawPDFPage(context, page);
    CGPDFDocumentRelease(document);
    
    CGDataProviderRef dataProvider = CGDataProviderCreateWithData(NULL, data, bytesPerPixel * height * width, NULL);
    CGImageRef img = CGImageCreate(width, height, bitsPerComponent, 32, bytesPerRow, colorSpace, kCGImageAlphaNone, dataProvider, NULL, FALSE, kCGRenderingIntentDefault);

    path = CFStringCreateWithCString(NULL, outFilename, CFStringGetSystemEncoding());
    url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, 0);
    CFStringRef pngUti = CFStringCreateWithCString(NULL, "public.png", CFStringGetSystemEncoding());
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, pngUti, 1, NULL);
    CFRelease(url);
    CFRelease(path);
    
    CGImageDestinationAddImage(destination, img, NULL);
    CGImageDestinationFinalize(destination);
    
    return 0;
}

