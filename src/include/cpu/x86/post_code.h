
#include <console/post_codes.h>


#if (CONFIG_POST_IO && CONFIG_EARLY_POST_IO) || (CONFIG_POST_IO && defined __ROMSTAGE__)
#define post_code(value)        \
	movb    $value, %al;    \
	outb    %al, $CONFIG_POST_IO_PORT

#else
#define post_code(value)
#endif
