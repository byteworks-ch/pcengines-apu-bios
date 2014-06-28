
#include <console/post_codes.h>


#if (CONFIG_IO_POST && CONFIG_EARLY_IO_POST) || (CONFIG_IO_POST && defined __ROMSTAGE__)
#define post_code(value)        \
	movb    $value, %al;    \
	outb    %al, $CONFIG_IO_POST_PORT

#else
#define post_code(value)
#endif
