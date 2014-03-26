#pragma once
#include "bcm_host.h"

typedef struct
{
    DISPMANX_DISPLAY_HANDLE_T   display;
    DISPMANX_MODEINFO_T         info;
    void                       *image;
    DISPMANX_UPDATE_HANDLE_T    update;
    DISPMANX_RESOURCE_HANDLE_T  resource;
    DISPMANX_ELEMENT_HANDLE_T   element;
    uint32_t                    vc_image_ptr;

} RECT_VARS_T;

static RECT_VARS_T  gRectVars;

class Mouse {
public:
    RECT_VARS_T    *vars;
    VC_IMAGE_TYPE_T type;
        
    Mouse(){
        type = VC_IMAGE_ARGB8888;
        int width=6;
        int height=6;
        
        uint32_t        screen = 0;
        int             ret;
        VC_RECT_T       src_rect;
        VC_RECT_T       dst_rect;

        VC_DISPMANX_ALPHA_T alpha = { (DISPMANX_FLAGS_ALPHA_T)(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS) , 
                                 255, //alpha 0->255
                                 0 };

        vars = &gRectVars;

        bcm_host_init();

        printf("Open display[%i]...\n", screen );
        vars->display = vc_dispmanx_display_open( screen );

        ret = vc_dispmanx_display_get_info( vars->display, &vars->info);
        assert(ret == 0);
        printf( "Display is %d x %d\n", vars->info.width, vars->info.height );

        vars->resource = vc_dispmanx_resource_create( type,
                                                      width,
                                                      height,
                                                      &vars->vc_image_ptr );
        assert( vars->resource );



        vars->update = vc_dispmanx_update_start( 10 );
        assert( vars->update );

        vc_dispmanx_rect_set( &src_rect, 0, 0, width << 16, height << 16 );

        // Full screen
        vc_dispmanx_rect_set( &dst_rect, 0, 0, width, height );

        vars->element = vc_dispmanx_element_add(    vars->update,
                                                    vars->display,
                                                    2000,               // layer
                                                    &dst_rect,
                                                    vars->resource,
                                                    &src_rect,
                                                    DISPMANX_PROTECTION_NONE,
                                                    &alpha,
                                                    NULL,             // clamp
                                                    DISPMANX_NO_ROTATE );

        
        vc_dispmanx_rect_set( &dst_rect, 0, 0, width, height);
        
        
        uint16_t *image = (uint16_t *)calloc( 1, width*4*height );
        memset(image, 0xFF, width*4*height);
        ret = vc_dispmanx_resource_write_data(  vars->resource,
                                                type,
                                                width*4,//image.step,
                                                image,
                                                &dst_rect );
        
        ret = vc_dispmanx_update_submit_sync( vars->update );
        assert( ret == 0 );
    }
    
    void move(int x, int y){
        int             ret;
        VC_RECT_T       dst_rect;
        
        vars->update = vc_dispmanx_update_start( 10 );
            
        vc_dispmanx_rect_set( &dst_rect, x, y, 6, 6);
        ret = vc_dispmanx_element_change_attributes(
            vars->update,
            vars->element,
            /*ELEMENT_CHANGE_DEST_RECT*/      (1<<2),
            0,
            0,
            &dst_rect,
            NULL,
            DISPMANX_NO_HANDLE,
            DISPMANX_NO_ROTATE);
        assert( ret == DISPMANX_SUCCESS );
        
        /* Submit asynchronously, otherwise the performance suffers a lot */
        ret = vc_dispmanx_update_submit( vars->update, 0, NULL );
        assert( ret == DISPMANX_SUCCESS );
    }
    void close(){
        int             ret;
        vars->update = vc_dispmanx_update_start( 10 );
        assert( vars->update );
        ret = vc_dispmanx_element_remove( vars->update, vars->element );
        assert( ret == 0 );
        ret = vc_dispmanx_update_submit_sync( vars->update );
        assert( ret == 0 );
        ret = vc_dispmanx_resource_delete( vars->resource );
        assert( ret == 0 );
        ret = vc_dispmanx_display_close( vars->display );
        assert( ret == 0 );
    }
};