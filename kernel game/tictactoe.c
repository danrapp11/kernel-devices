#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

// static ints storing the index of the coordinates in the game_board string
static int A1 = 26;
static int A2 = 37;
static int A3 = 48;
static int B1 = 28;
static int B2 = 39;
static int B3 = 50;
static int C1 = 30;
static int C2 = 41;
static int C3 = 52;

// list of ai_turns in order, used in ai_turn function
static const int ai_turns[] = {26,28,30,37,39,41,48,50,52};

// static char of empty board, used to reset board to empty
static char empty_board[55] = {
    "- - A B C \n  --------\n1 | - - - \n2 | - - - \n3 | - - - \n"
};

static char *reset_string = "$ RESET\nOK\n";
static char *board_string = "$ BOARD\n";
static char *turn_string = "$ ";
static char *invalid_input = "Invalid input detected\n";
static char *invalid_move = "Invalid move\n";
static char *player_win = "You win! Input RESET to start new game\n";
static char *ai_win = "AI wins! Input RESET to start new game\n";
static char *output = "";
static char game_board[55];
static char input[16] = "";
static ssize_t input_len = 0;
static size_t output_size = 0;
static int game_over;

// open function, prints to kernel when opened and returns 0
static int my_open(struct inode *inode, struct file *file) {
//    printk(KERN_ALERT "Opening tictactoe\n");
    strncpy(input, "", 1);
    input_len = 0;
    return 0;
}

// release function, prints to kernel when released and increments quote number
static int my_release(struct inode *inode, struct file *file) {
//    printk(KERN_ALERT "Closing tictactoe\n");
    return 0;
}

// read function which uses copy to user function to print quote to userspace
static ssize_t my_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset) {
    ssize_t length = strlen(output);
    if (*offset >= length)
	return 0;
    if (size > length - *offset)
	size = length - *offset;
    if (copy_to_user(user_buffer, output, size))
	return -EFAULT;
    *offset = *offset + size;
    return size;
}

// takes in input of command (ex. "TURN A 1"), parses command into array of strings, returns -1 on invalid input or int value representing
// the index of the given coordinates in the game board (TURN A 1 would return 26 since that is the index of the A 1 spot on the board).
int get_turn(char *command_line) {
    char *parsed_command[3] = {0};
    char *word;
    char *first;
    char *second;
    size_t parsed_command_size;
    size_t length1;
    size_t length2;
    int turn;
    int i = 0;
    // parse command and store each token in array parsed_command (TURN A 1 = "TURN", "A", "1")
    while ((word = strsep(&command_line, " ")) != NULL) {
	// if there are more than 3 tokens in the list, return error
	if (i > 2)
	    return -1;
	parsed_command[i] = word;
	i++;
    }
    parsed_command_size = sizeof(parsed_command) / sizeof(parsed_command[0]);
    // if parsed_command contains more than 3 elements, sort of redundant since checked during parse, but want to be thorough
    if (parsed_command_size < 3)
	return -1;
    length1 = strlen(parsed_command[1]);
    length2 = strlen(parsed_command[2]);
    // if the coordinate values are more than 1 character long, return error
    // length2 is > 2 instead of > 1 because strsep includes a \n character at the end of last token for some reason
    if (length1 > 1 || length2 > 2)
	return -1;
    first = parsed_command[1];
    second = parsed_command[2];
    // checks to make sure format is correct, first coord is A B or C and second is 1 2 or 3 otherwise, return error
    if ((strstr(first, "A") == NULL) && (strstr(first, "B") == NULL) && (strstr(first, "C") == NULL))
	return -1;
    if ((strstr(second, "1") == NULL) && (strstr(second, "2") == NULL) && (strstr(second, "3") == NULL))
        return -1;
    // past error checking, now return index in game_board of given move
    if (strstr(first, "A") != NULL) {
	if (strstr(second, "1") != NULL)
	    turn = A1;
	else if (strstr(second, "2") != NULL)
	    turn = A2;
	else
	    turn = A3;
    }
    else if (strstr(first, "B") != NULL) {
        if (strstr(second, "1") != NULL)
            turn = B1;
        else if (strstr(second, "2") != NULL)
            turn = B2;
        else
            turn = B3;
    }
    else {
        if (strstr(second, "1") != NULL)
            turn = C1;
        else if (strstr(second, "2") != NULL)
            turn = C2;
        else
            turn = C3;
    }
    return turn;
}

// takes input of index on board and checks to see if the spot is empty, returns -1 if spot is taken
int check_turn(int turn) {
    if (game_board[turn] == '-')
	return 0;
    else
	return -1;
}

// checks each possible way player could win and returns 1 if one of those possibilites are met
int check_player_win(void) {
    int ret = 0;
    if (game_board[A1] == 'X' && game_board[A2] == 'X' && game_board[A3] == 'X')
	ret = 1;
    else if (game_board[B1] == 'X' && game_board[B2] == 'X' && game_board[B3] == 'X')
        ret = 1;
    else if (game_board[C1] == 'X' && game_board[C2] == 'X' && game_board[C3] == 'X')
	ret = 1;
    else if (game_board[A1] == 'X' && game_board[B1] == 'X' && game_board[C1] == 'X')
        ret = 1;
    else if (game_board[A2] == 'X' && game_board[B2] == 'X' && game_board[C2] == 'X')
        ret = 1;
    else if (game_board[A3] == 'X' && game_board[B3] == 'X' && game_board[C3] == 'X')
        ret = 1;
    else if (game_board[A1] == 'X' && game_board[B2] == 'X' && game_board[C3] == 'X')
        ret = 1;
    else if (game_board[A3] == 'X' && game_board[B2] == 'X' && game_board[C1] == 'X')
        ret = 1;
    return ret;
}

// same as player win, just checks for 'O' instead of 'X'
int check_ai_win(void) {
    int ret = 0;
    if (game_board[A1] == 'O' && game_board[A2] == 'O' && game_board[A3] == 'O')
        ret = 1;
    else if (game_board[B1] == 'O' && game_board[B2] == 'O' && game_board[B3] == 'O')
        ret = 1;
    else if (game_board[C1] == 'O' && game_board[C2] == 'O' && game_board[C3] == 'O')
        ret = 1;
    else if (game_board[A1] == 'O' && game_board[B1] == 'O' && game_board[C1] == 'O')
        ret = 1;
    else if (game_board[A2] == 'O' && game_board[B2] == 'O' && game_board[C2] == 'O')
        ret = 1;
    else if (game_board[A3] == 'O' && game_board[B3] == 'O' && game_board[C3] == 'O')
        ret = 1;
    else if (game_board[A1] == 'O' && game_board[B2] == 'O' && game_board[C3] == 'O')
        ret = 1;
    else if (game_board[A3] == 'O' && game_board[B2] == 'O' && game_board[C1] == 'O')
        ret = 1;
    return ret;
}

// function for ai turn, just plays in order left to right up to down, no smart algorithm
void ai_turn(void) {
    int i = 0;
    int turn;
    while (i < 9) {
	turn = ai_turns[i];
	if (check_turn(turn) == 0) {
	    game_board[turn] = 'O';
	    return;
	}
	i++;
    }
}

// write function which copys info from userspace into string variable input
static ssize_t my_write(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset) {
    ssize_t bits = 0;
    int turn;
    if (size > 16 - input_len)
	return -ENOSPC;
    if (copy_from_user(input + input_len, user_buffer, size))
	return -EFAULT;
    input_len = input_len + size;
    bits = size;
    // if the input contains RESET, reset the game board to empty
    if (strstr(input, "RESET") != NULL) {
	game_over = 0;
	output_size = output_size + strlen(reset_string);
	strncpy(game_board, empty_board, 55);
	output = krealloc(output, output_size, GFP_KERNEL);
	strncat(output, reset_string, strlen(reset_string));
	return bits;
    }
    // if the input contains BOARD, print the board to kernel space
    else if (strstr(input, "BOARD") != NULL) {
	output_size = output_size + strlen(board_string) + 55;
	output = krealloc(output, output_size, GFP_KERNEL);
	strncat(output, board_string, strlen(board_string));
	strncat(output, game_board, 55);
	return bits;
    }
    // if the input contains TURN, which calls get_turn, which returns -1 if given an invalid move
    else if (strstr(input, "TURN") != NULL) {
	output_size = output_size + strlen(turn_string) + strlen(input);
	output = krealloc(output, output_size, GFP_KERNEL);
	strncat(output, turn_string, strlen(turn_string));
	strncat(output, input, strlen(input));
	if (game_over == 1)
	    return bits;
	turn = get_turn(input);
	// if get_turn returns -1, invalid input, if check_turn returns -1, space it taken up already
	if (turn < 0 || check_turn(turn) < 0) {
	    output_size = output_size + strlen(invalid_move);
            output = krealloc(output, output_size, GFP_KERNEL);
            strncat(output, invalid_move, strlen(invalid_move));
	    return bits;
	}
	// make player move and check if they won the game
	game_board[turn] = 'X';
	if (check_player_win() == 1) {
	    output_size = output_size + strlen(player_win);
	    output = krealloc(output, output_size, GFP_KERNEL);
	    strncat(output, player_win, strlen(player_win));
	    game_over = 1;
	    return bits;
	}
	// make ai move and check if they won the game
	ai_turn();
	if (check_ai_win() == 1) {
	    output_size = output_size + strlen(ai_win);
            output = krealloc(output, output_size, GFP_KERNEL);
            strncat(output, ai_win, strlen(ai_win));
	    game_over = 1;
	    return bits;
	}
	return bits;
    }
    else {
	output_size = output_size + strlen(turn_string) + strlen(input) + strlen(invalid_input);
	output = krealloc(output, output_size, GFP_KERNEL);
	strncat(output, turn_string, strlen(turn_string));
	strncat(output, input, strlen(input));
	strncat(output, invalid_input, strlen(invalid_input));
	return bits;
    }
}

// object containing all file operations above
const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write
};

// struct for miscdevice that contains name, file_ops and the permissions the device will be assigned
struct miscdevice my_misc = {
    .name = "tictactoe",
    .fops = &my_fops,
    .mode = 0666 //read and write permissions
};

// initialization function which uses the misc_register function to register and create the device "inspiration" and returns message on failure
// sudo insmod ./tictactoe.ko
static int __init quote_init(void) {
    int ret;
    printk("Initializing tictactoe module\n");
    ret = misc_register(&my_misc);
    if (ret < 0) {
        printk(KERN_ALERT "tictactoe misc registration failed\n");
        return ret;
    }
    output = kmalloc(output_size, GFP_KERNEL);
    return ret;
}

// exit function which deregisters device (sudo rmmod tictactoe)
static void __exit quote_exit(void) {
    misc_deregister(&my_misc);
    kfree(output);
    printk(KERN_ALERT "Exiting tictactoe module\n");
}

module_init(quote_init);
module_exit(quote_exit);
