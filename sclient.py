import socket
import sys
import random
from ctypes import *
import time
import tkinter
import keyboard

CANVAS_WIDTH = 600  # Width of drawing canvas in pixels
CANVAS_HEIGHT = 600  # Height of drawing canvas in pixels
UNIT_SIZE = 10  # Decides how thick the snake is
SPEED = 10  # Greater value here increases the speed of motion of the snakes

""" This class defines a C-like struct """
class IntialData(Structure):
    _fields_ = [("CANVAS_WIDTH", c_int),
                ("CANVAS_HEIGHT", c_int),
                ("UNIT_SIZE", c_int),
                ("SPEED", c_int),
                ("SNAKE_ID", c_int)]

class FoodPos(Structure):
    _fields_ = [("x1", c_int), ("y1", c_int)]

class Control(Structure):
    _fields_ = [("x", c_int)]

class Send_data(Structure):
    _fields_ = [("id", c_uint8),
                ("type", c_uint8)]

class Recv_control(Structure):
    _fields_ = [("control0", c_uint8),
                ("control1", c_uint8)]
class GameState(Structure):
    _fields_ = [("food_x", c_int16),
                ("food_y", c_int16),
                ("counter0", c_int32),
                ("counter1", c_int32),
                ("score0", c_int16),
                ("score1", c_int16),
                ("is_alive0", c_int8),              
                ("is_alive1", c_int8),
                ("is_col", c_int8)]
class Snake:
    """
    This class defines the properties of a snake object for the game and contains methods for creating the snake,
    dynamically increasing its size and its movements
    """

    def __init__(self, snake_num, canvas, snake_color):
        self.direction_x = 1
        self.direction_y = 0
        self.start_snake_size = 10
        self.snake_size_counter = 0
        self.score_counter = 0
        self.snake_chain = []
        self.snake_num = snake_num
        self.canvas = canvas
        self.snake_color = snake_color
        self.start_x = (self.start_snake_size - 1)*UNIT_SIZE
        if self.snake_num == 1:
            self.start_y = CANVAS_HEIGHT / 3 - UNIT_SIZE
        else:
            self.start_y = (CANVAS_HEIGHT * 2 / 3) - UNIT_SIZE
        self.initialize_snake()
        self.is_alive = True

    def initialize_snake(self):
        """
         Method to instantiate the initial snake :
         Each Snake is instantiated as a chain of squares appearing as a single long creature.
         This method creates a circular head(tagged as 'snake_<num>' and 'head' for future reference)
         and n no.of blocks based on start_snake_size.
         Each snake block is stored as an object in the list snake_chain[]
        """
        #snake head
        self.snake_chain.append(self.canvas.create_oval(self.start_x, self.start_y,
                                                        self.start_x + UNIT_SIZE, self.start_y + UNIT_SIZE,
                                                        fill='orange', outline='brown',
                                                        tags=('snake_' + str(self.snake_num), 'head')))
        #snake body
        for blockIndex in range(self.start_snake_size - 1):
            x0 = (self.start_snake_size - 1 - (blockIndex + 1)) * UNIT_SIZE
            x1 = x0 + UNIT_SIZE
            snake_block = self.create_snake_block(x0, self.start_y, x1, self.start_y + UNIT_SIZE,
                                                  self.start_snake_size - 1 - blockIndex)
            self.snake_chain.append(snake_block)

    def create_snake_block(self, x0, y0, x1, y1, tag):
        """
         Method to create a single block of each snake based on the coordinates passed to it.
         Each block is tagged as 'snake' to be accessed in future.
        """
        return self.canvas.create_rectangle(x0, y0, x1, y1, fill=self.snake_color, tags='snake')

    '''
     move_* methods below control the snake's navigation. These functions are invoked based on user's key presses.
     Special checks are done in each of them to ensure invalid turns are blocked 
     (Ex: Block right turn if the snake is currently going to the left, and so on)
    '''
    def move_up(self):
        if self.direction_x == 0 and self.direction_y == 1:
            pass
        else:
            self.direction_y = -1
            self.direction_x = 0

    def move_down(self):
        if self.direction_x == 0 and self.direction_y == -1:
            pass
        else:
            self.direction_y = 1
            self.direction_x = 0

    def move_left(self):
        if self.direction_x == 1 and self.direction_y == 0:
            pass
        else:
            self.direction_x = -1
            self.direction_y = 0

    def move_right(self):
        if self.direction_x == -1 and self.direction_y == 0:
            pass
        else:
            self.direction_x = 1
            self.direction_y = 0

    def plus_size(self):
        """
        This method increments the snake size by '1', by adding a block to it.
        The snake head co-ordinates are grabbed and used to decide the new block's coordinates based on current size
        """
        #self.snake_size_counter += 1
        x0, y0, x1, y1 = self.canvas.coords(self.snake_chain[0])
        if self.direction_x == 1:
            x0 -= self.snake_size_counter * UNIT_SIZE
            x1 -= self.snake_size_counter * UNIT_SIZE
        elif self.direction_x == -1:
            x0 += self.snake_size_counter * UNIT_SIZE
            x1 += self.snake_size_counter * UNIT_SIZE
        elif self.direction_y == 1:
            y0 -= self.snake_size_counter * UNIT_SIZE
            y1 -= self.snake_size_counter * UNIT_SIZE
        elif self.direction_y == -1:
            y0 += self.snake_size_counter * UNIT_SIZE
            y1 += self.snake_size_counter * UNIT_SIZE
        snake_block = self.create_snake_block(x0, y0, x1, y1, (self.snake_size_counter + self.start_snake_size))
        self.snake_chain.append(snake_block)  # Whenever a new block is added to snake, add it to snake list.

    def move_snake(self):
        """
        In each frame, the snake's position is grabbed in a dictionary chain_pos_dict{}.
        'Key:Value' pairs here are each of the 'Snake_block(Object ID):Its coordinates'.
        Algorithm to move snake:
        1) The ‘snake head’ is repositioned based on the player controls.
        2) The block following the snake head is programmed to take
        snake head’s previous position in the subsequent frame.
        Similarly, the 3rd block takes the 2nd block position and so on.
        """
        if self.is_alive:
            chain_pos_dict = {}
            for obj in self.snake_chain:
                chain_pos_dict[obj] = self.canvas.coords(obj)

            snake_head_tag = self.get_head_tag()
            self.canvas.move(snake_head_tag, self.direction_x * UNIT_SIZE, self.direction_y * UNIT_SIZE)

            '''
            ASSUMPTION : Object IDs are maintained in a sorted list assuming that 
            the IDs are assigned in increasing order for new objects instantiated on Canvas
            '''
            key_list = sorted(chain_pos_dict.keys())
            nI = len(key_list)
            for i in range(1, nI):
                self.canvas.moveto(key_list[i], chain_pos_dict[key_list[i - 1]][0] - 1,
                                   chain_pos_dict[key_list[i - 1]][1] - 1)

    def get_head_tag(self):
        return 'snake_' + str(self.snake_num) + '&&head'

class TkinkerCanvas:
    """
    Creates a canvas and contains attributes for all the objects on the Canvas(food, score_board, etc).
    The methods in it handle everything for the game right from instantiating the snakes, score_board to
    handling player controls, placing food, processing events happening during the game
    """
    def __init__(self, top):
        self.top = top
        self.canvas = self.make_canvas(CANVAS_WIDTH, CANVAS_HEIGHT, 'Snake Game')
        #self.player_controls = ['<Up>', '<Down>', '<Left>', '<Right>']
        #self.snake = Snake(1, self.canvas, 'brown')
        #self.set_player_control(self.snake, self.player_controls)
        #self.score_board = self.create_score_board(self.snake.snake_num, self.snake.snake_color)
        self.start_game()

    def make_canvas(self, width, height, title):
        """
        Method to create a canvas that acts as a base for all the objects in the game
        """
        self.top.minsize(width=width, height=height)
        self.top.title(title)

        canvas = tkinter.Canvas(self.top, width=width + 1, height=height + 1, bg='black')
        canvas.pack(padx=10, pady=10)
        return canvas
    def snakes_init(self, snake_num):
        self.snake0 = Snake(0, self.canvas, 'brown')
        self.snake1 = Snake(1, self.canvas, 'green')
        self.score_board0 = self.create_score_board(self.snake0.snake_num, self.snake0.snake_color)
        self.score_board1 = self.create_score_board(self.snake1.snake_num, self.snake1.snake_color)
        if snake_num == 0:
            self.snake = self.snake0
        else:
            self.snake = self.snake1
    def set_player_control(self, snake, player_controls):
        """
        Method to bind keyboard inputs on canvas to specific functions for navigating the snake
        """
        self.canvas.focus_set()
        self.canvas.bind(player_controls[0], snake.move_up)
        self.canvas.bind(player_controls[1], snake.move_down)
        self.canvas.bind(player_controls[2], snake.move_left)
        self.canvas.bind(player_controls[3], snake.move_right)
    def move_snake(self, snake : Snake, control):
        if control != -6:
            if control == 0:
                snake.move_up()
            if control == 1:
                snake.move_down()
            if control == 2:
                snake.move_left()
            if control == 3:
                snake.move_right()

    def create_score_board(self, num, color):
        """
        Method to position score_board text on the canvas for each player
        """
        if num == 1:
            x_offset = 0.05
        else:
            x_offset = 0.85
        return self.canvas.create_text(x_offset * CANVAS_WIDTH, 0.01 * CANVAS_HEIGHT,
                                       text=('Score : ' + str(0)), anchor=tkinter.NW,
                                       font=("Times", 12, 'bold'), fill=color)

    def hit_something(self, snake : Snake):
        """
        Method to handle events during the Snake's motion.
        Makes use of 'tags' given to each object to filter out what's overlapping.
        1. Hit food --> Check if the hit object is food: If yes, eat it, increment snake size and delete food object
        2. Hit wall --> Check if Snake head went past the wall coordinates: If yes, kill snake
        3. Hit snake --> Check if Snake hit itself or other snake: If yes, kill this snake
        """
        snake_head_tag = snake.get_head_tag()
        x1, y1, x2, y2 = self.canvas.coords(snake_head_tag)

        if (x1 <= 0) or (y1 <= 0) or (x2 >= CANVAS_WIDTH) or (y2 >= CANVAS_HEIGHT):
            self.handle_hit_wall(snake)
            return 0

        results = self.canvas.find_overlapping(x1+1, y1+1, x2-1, y2-1)
        for item in results:
            if 'food' in self.canvas.gettags(item):
                print(self.food_id)
                self.handle_hit_food(item, snake)
                return 1
                #break
            elif 'snake' in self.canvas.gettags(item):
                self.handle_hit_snake(snake)
                return 2
        
        self.update_gameState(7)

    def handle_hit_snake(self, snake : Snake):
        self.update_gameState(5)
        #snake.is_alive = False

    def handle_hit_food(self, food_id, snake : Snake):
        #self.canvas.delete(food_id)
        self.update_gameState(4)
        #snake.plus_size()
        #snake.score_counter += 10
        #self.place_food(self.food_x, self.food_y)

    def handle_hit_wall(self, snake : Snake):
        self.update_gameState(5)
        #snake.is_alive = False

    def handle_game_over(self):
        """
        Method to print out the final message and declare the winner based on player scores
        """
        print("Game Over!")
        if not self.snake0.is_alive:
            result_msg = self.snake1.snake_color.title() + ' Snake wins!'
        elif not self.snake1.is_alive:
            result_msg = self.snake0.snake_color.title() + ' Snake wins!'
        else:
            if self.snake0.score_counter > self.snake1.score_counter:
                result_msg = self.snake0.snake_color.title() + ' Snake wins!'
            elif self.snake1.score_counter > self.snake0.score_counter:
                result_msg = self.snake1.snake_color.title() + ' Snake wins!'
            else:
                result_msg = 'Its a tie!'
        widget = tkinter.Label(self.canvas, text='Game Over!\n' + result_msg,
                               fg='white', bg='black', font=("Times", 20, 'bold'))
        widget.pack()
        widget.place(relx=0.5, rely=0.5, anchor='center')

    def handle_collision(self):
        self.canvas.delete(self.food_id)
        if self.is_col == 1:
            self.snake0.plus_size()
        else:
            self.snake1.plus_size()

        self.place_food(self.food_x, self.food_y)

    def is_game_over(self):
        if not self.snake0.is_alive or not self.snake1.is_alive:
            self.handle_game_over()
            return True
        return False

    def place_food(self, x1, y1):
        """
        Method to randomly place a circular 'food' object anywhere on Canvas.
        The tag on it is used for making decisions in the program
        """
        #x1 = random.randrange(2*UNIT_SIZE, CANVAS_WIDTH - UNIT_SIZE, step=UNIT_SIZE)
        #y1 = random.randrange(2*UNIT_SIZE, CANVAS_HEIGHT - UNIT_SIZE, step=UNIT_SIZE)
        print("\nplace food here\n")
        self.food_id = self.canvas.create_oval(x1, y1, x1 + UNIT_SIZE, y1 + UNIT_SIZE, fill='red', tags='food')

    def place_food_start(self):
        x1 = random.randrange(2*UNIT_SIZE, CANVAS_WIDTH - UNIT_SIZE, step=UNIT_SIZE)
        y1 = random.randrange(2*UNIT_SIZE, CANVAS_HEIGHT - UNIT_SIZE, step=UNIT_SIZE)
        print("\nplace food here\n")
        print(x1)
        print(y1)
        self.canvas.create_oval(x1, y1, x1 + UNIT_SIZE, y1 + UNIT_SIZE, fill='red', tags='food')
    def update_scores(self):
        self.canvas.itemconfig(self.score_board0, text='Score : ' + str(self.snake0.score_counter))
        self.canvas.itemconfig(self.score_board1, text='Score : ' + str(self.snake1.score_counter))

    def display_label(self, message, display_time):
        """
        Method to display introductory messages on screen before the start of the game
        """
        widget = tkinter.Label(self.canvas, text=message, fg='white', bg='black',
                               font=("Times", 20, 'bold'))
        widget.place(relx=0.5, rely=0.5, anchor='center')
        self.canvas.update()
        time.sleep(display_time)
        widget.place_forget()
        self.canvas.update()

    def starter_message(self):
        if(self.snake.snake_num == 0):
            self.display_label('Welcome to the Snake World!, press UP, DOWN, LEFT, RIGHT to move', 3)
        else:
            self.display_label('Welcome to the Snake World!, press W, S, A, D to move', 3)
        self.display_label('Your game starts in \n 3', 1)
        self.display_label('Your game starts in \n 2', 1)
        self.display_label('Your game starts in \n 1', 1)

    def recv_msg(self):
        msg = self.s.recv(sizeof(c_char) * 3)
        msg = msg.decode("utf-8") 
        print("[DEBUG] Received msg: {}\n".format(msg))
        return msg

    def recv_int(self):
        msg = self.s.recv(sizeof(c_int))
        int_value = int.from_bytes(msg, "little")    
        print("[DEBUG] Received int: {}\n".format(int_value))

        return int_value
    def open_socket(self, serv_addr ,port):
        server_addr = (serv_addr, port)
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        return server_addr, s

    def sendControl(self, signal):
        print("Sending")
        nsent = self.s.send(Send_data(self.snake.snake_num, signal))
        # Alternative: s.sendall(...): coontinues to send data until either
        # all data has been sent or an error occurs. No return value.
        print("Sent {:d}, {:d}".format(self.snake.snake_num,signal))

        buff = self.s.recv(sizeof(Recv_control))
        payload_in = Recv_control.from_buffer_copy(buff)
        #print("Received control x={:d}".format(payload_in.x))
        return payload_in.control0, payload_in.control1 

    def sendCollision(self, signal):
        print("Sending")
        nsent = self.s.send(Send_data(self.snake.snake_num, signal))
        # Alternative: s.sendall(...): coontinues to send data until either
        # all data has been sent or an error occurs. No return value.
        print("Sent {:d} bytes".format(nsent))

        buff = self.s.recv(sizeof(GameState))
        payload_in = GameState.from_buffer_copy(buff)
        return payload_in.food_x, payload_in.food_y , payload_in.counter0,  payload_in.counter1, payload_in.score0,  payload_in.score1, payload_in.is_alive0, payload_in.is_alive1, payload_in.is_col

    def control_detection(self):
        if(self.snake.snake_num == 0):
            if keyboard.is_pressed("up"):
                return self.sendControl(0)
            if keyboard.is_pressed("down"):
                return self.sendControl(1)
            if keyboard.is_pressed("left"):
                return self.sendControl(2)
            if keyboard.is_pressed("right"):
                return self.sendControl(3)
        else:
            if keyboard.is_pressed("w"):
                return self.sendControl(0)
            if keyboard.is_pressed("s"):
                return self.sendControl(1)
            if keyboard.is_pressed("a"):
                return self.sendControl(2)
            if keyboard.is_pressed("d"):
                return self.sendControl(3)
        return self.sendControl(6)
    def update_gameState(self, signal):
        self.food_x, self.food_y, self.snake0.snake_size_counter, self.snake1.snake_size_counter, self.snake0.score_counter, self.snake1.score_counter, self.snake0.is_alive,  self.snake1.is_alive, self.is_col = self.sendCollision(signal)
    def start_game(self):
        """
        The heart of the game : the 'while True' animation loop
        Keeps moving the snake and updating the canvas.
        Checks for events like hit food/self/other snake/wall to keep updating snake size/scores, etc
        until game is over
        """
        #
        #receive intial state of the game
        server_addr, self.s = self.open_socket('localhost', 2300)
        try:
            self.s.connect(server_addr)
            print("Connected to {:s}".format(repr(server_addr)))
            print("")
            print("Waiting for other player")
            snake_id = self.recv_int()
            #wait for the start signal
            #buff = self.s.recv(4)
            ############################
            msg = self.recv_msg()
            if msg == "HLD":
                print("Waiting for other player")   
            while msg != "STR":
                msg = self.recv_msg()
                if msg == "HLD":
                    print("Waiting for other player")   

            #intial_data = IntialData(0, 0, 0, 0, 0)
            #print("Sending")
            #nsent = self.s.send(intial_data)
            #print("Sent {:d} bytes".format(nsent))
            
            #receive the intitalized data and create snake 
            buff = self.s.recv(sizeof(IntialData))
            payload_in = IntialData.from_buffer_copy(buff)
            print("Received width={:d}, height={:d}, unit={:d}, speed={:d}, id-{:d}".format(payload_in.CANVAS_WIDTH,
                                                                payload_in.CANVAS_HEIGHT,
                                                                payload_in.UNIT_SIZE,
                                                                payload_in.SPEED,
                                                                payload_in.SNAKE_ID))
            self.CANVAS_WIDTH = payload_in.CANVAS_WIDTH  # Width of drawing canvas in pixels
            self.CANVAS_HEIGHT = payload_in.CANVAS_HEIGHT  # Height of drawing canvas in pixels
            self.UNIT_SIZE = payload_in.UNIT_SIZE  # Decides how thick the snake is
            self.SPEED = payload_in.SPEED  # Greater value here increases the speed of motion of the snakes  
            snake_id = payload_in.SNAKE_ID
            self.snakes_init(snake_id)
            ##############################################

            #food_pos= FoodPos(0,0)
            #print("Sending")
            #nsent = self.s.send(food_pos)
            #print("Sent {:d} bytes".format(nsent))

            #init the first food position
            buff = self.s.recv(sizeof(FoodPos))
            payload_in = FoodPos.from_buffer_copy(buff)
            print("Received food postion x={:d}, y={:d}".format(payload_in.x1,
                                                                payload_in.y1))
            ################################
            self.starter_message()
            # The first food is placed outside the loop to kick start the game
            self.place_food(payload_in.x1, payload_in.y1)
            
            #self.place_food_start()

            # Animation Loop
            while True:
                # Update World

                #send control signal to server
                control0, control1 = self.control_detection()            
                #receiv control signal from server
                #control0, control1 = recv_control_from_server()
                self.move_snake(self.snake0, control0)
                self.move_snake(self.snake1, control1)
                self.snake0.move_snake()
                self.snake1.move_snake()
                
                self.canvas.update()

                self.hit_something(self.snake)
               
                if self.is_col != 0:
                    self.handle_collision()
                #send hit signal and receive score, game_state
                #self.update_gameState()
                #receive score, game state
                self.update_scores()
                if self.is_game_over():
                    break
                # pause
                time.sleep(1/SPEED)  # Time to hold each frame; reducing this time gives a notion of increased snake speed
        except AttributeError as ae:
            print("Error creating the socket: {}".format(ae))
        except socket.error as se:
            print("Exception on socket: {}".format(se))
        finally:
            print("Closing socket")
            self.s.close()
            
def main():
    top = tkinter.Tk()
    TkinkerCanvas(top)
    top.mainloop()
    
if __name__ == '__main__':
    main()
