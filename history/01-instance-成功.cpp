#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <string.h>


int main(){

	Display *display;
	Window window;
	Window root_window, child_window;
	XEvent event;
	KeySym event_key;
	GC gc1, gc2;
	XFontStruct *font_info;
	
	char *input;
	int screen;
	int root_x, root_y, win_x, win_y;
	unsigned int mask;


	int command_mode = 0;
	int input_count = 0;
	char input_temp[256];
	input_temp[0] = ':';
	char *text = "Hello";

	char mouse_location[20];


	display = XOpenDisplay(NULL);
	if(display == NULL){
		fprintf(stderr, "無法連接到x伺服器\n");
		exit(1);
	}

	screen = DefaultScreen(display);


	root_window = RootWindow(display, screen);
	window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, 200, 200, 1, BlackPixel(display, screen), WhitePixel(display, screen));

	XStoreName(display, window, "flow");

	XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | PointerMotionMask);
	XMapWindow(display, window);


	gc1 = XCreateGC(display, window, 0, NULL);
	gc2 = XCreateGC(display, window, 0, NULL);


	XSetForeground(display, gc2, WhitePixel(display, 0));
	XSetBackground(display, gc2, WhitePixel(display, 0));

	font_info = XLoadQueryFont(display, "fixed");
	if(!font_info){
		fprintf(stderr, "無法加載字體");
		exit(1);
	}
	XSetFont(display, gc1, font_info->fid);


	while(1){
		XNextEvent(display, &event);
		switch(event.type){
			case Expose:
				break;
			case KeyPress:
				event_key = XLookupKeysym(&(event.xkey), 0);
				if(command_mode == 1){
					switch(event_key){
						case XK_Escape:
							command_mode = 0;
							printf("\n");
							break;
						case XK_Return:
							command_mode = 0;
							printf("\n");
							if(input_temp[1] == 'q' && input_temp[2] == '\0'){
								XCloseDisplay(display);
								return 0;
							}
							break;
						default:
							input = XKeysymToString(event_key);
							input_temp[input_count] = *input;
							input_temp[input_count + 1] = '\0';
							input_count++;
							printf("\r%s", input_temp);
							fflush(stdout);
							break;
					}
				}
				else{
					switch(event_key){
						case XK_semicolon:
							if(event.xkey.state & ShiftMask){
								command_mode = 1;
								input_count = 1;
								input_temp[1] = '\0';
								printf("\r%s", input_temp);
							}
							break;
						default:
							break;
					}
				}
				break;
			case KeyRelease:
			//	event_key = XLookupKeysym(&(event.xkey), 0);
			//	input = XKeysymToString(event_key);
			//	printf("放開鍵盤 %s\n", input);
				break;
			case ButtonPressMask:
				switch(event.xbutton.button){
					case 1:
						XQueryPointer(display, window, &root_window, &child_window, &root_x, &root_y, &win_x, &win_y, &mask);
						printf("按下左鍵 %d %d\n", win_x, win_y);
						break;
					case 2:
						printf("按下中鍵\n");
						break;
					case 3:
						printf("按下右鍵\n");
						break;
					case 4:
						printf("滾輪向上\n");
						break;
					case 5:
						printf("滾輪向下\n");
						break;
					default:
						printf("按下滑鼠\n");
				}
				break;
			case MotionNotify:
				XQueryPointer(display, window, &root_window, &child_window, &root_x, &root_y, &win_x, &win_y, &mask);
				//printf("滑鼠移動 %d %d\n", win_x, win_y);
				sprintf(mouse_location, "%d %d", win_x, win_y);
				XClearArea(display, window, 0, 0, 100, 20, False);
				XDrawString(display, window, gc1, 10, 20, mouse_location, strlen(mouse_location));

				break;
			default:
				printf("event\n");
				break;
		}

		fflush(stdout);
	}

	XCloseDisplay(display);
	printf("123\n");
	return 0;
}
