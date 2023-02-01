import tkinter
import socket
import time
from ctypes import *
CANVAS_WIDTH = 5  # Width of drawing canvas in pixels
CANVAS_HEIGHT = 5  # Height of drawing canvas in pixels

class TkinkerCanvas:
    """
    Creates a canvas and contains attributes for all the objects on the Canvas.
    The methods in it handle everything for the game right from instantiating the board, score_board to
     processing events happening during the game
    """
    def __init__(self, top):
        #self.top = top
        #self.canvas = self.make_canvas(CANVAS_WIDTH, CANVAS_HEIGHT, 'Bingo Game')
        self.screen = top
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
    def make_grid(self):
        def on_click(value):
            print()
            self.write_server_int(value)
            self.wait = False
        self.label = tkinter.Label(width=52,height=2,text="Wait for other turn,...",bg='grey', fg='white')
        self.label.grid(row=5,columnspan=5)   
        self.grids = [None] * 25
        for i in range(25):                      
            self.grids[i] = tkinter.Button(
                                height = 2, width = 4,
                                font = ("Helvetica","20"),
                                text = self.board[i],
                                command = lambda  value = self.board[i] : on_click(value))
            row = int(i%5)
            column =  int(i/5)     
            self.grids[i].grid(row = row, column = column)
    def recv_board(self):
        msg = self.s.recv(sizeof(c_int) * 50)
        self.board = []
        for i in range(0, len(msg), 4):
            self.board.append(int.from_bytes(bytes(msg[i:i+2]), 'little')) 
        #for i in range(sizeof(c_int) * 50):
        #    if i % 3 == 0:
        #        self.board.append(msg[i])
        #self.board = [x for x in msg]
        print(self.board)
    def draw_board(self):
        for i in range(25):
            if i != 0 and i % 5 == 0:
                print("\n------------------------\n", end =" ")
                print("| {} |".format(self.board[i]),  end =" ")
            else:
                print("| {} |".format(self.board[i]),  end =" ")
            self.grids[i].configure(state = tkinter.DISABLED)                
            if self.board[i] == 65535:
                self.grids[i].configure(bg = 'red')
        self.screen.update()
        print("\n")
            
    def starter_message(self):
        self.display_label('Welcome to the Bingo World!', 3)
        self.display_label('Your game starts in \n 3', 1)
        self.display_label('Your game starts in \n 2', 1)
        self.display_label('Your game starts in \n 1', 1)

    def open_socket(self, serv_addr ,port):
        server_addr = (serv_addr, port)
        sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        return server_addr, sockfd
    
    def recv_int(self):
        msg = self.s.recv(sizeof(c_int))
        int_value = int.from_bytes(msg, "little")    
        print("[DEBUG] Received int: {}\n".format(int_value))

        return int_value

    def recv_msg(self):
        msg = self.s.recv(sizeof(c_char) * 3)
        print(msg)
        msg = msg.decode("utf-8") 
        print("[DEBUG] Received msg: {}\n".format(msg))
        return msg

    def write_server_int(self, msg):
        nsent = self.s.send(c_int(msg))
        # Alternative: s.sendall(...): coontinues to send data until either
        # all data has been sent or an error occurs. No return value.
        print("Sent {:d} bytes".format(nsent))
        print("[DEBUG] Wrote int to server: {}\n".format(msg))
    

    def update_gameState(self, signal):
        self.food_x, self.food_y, self.snake0.snake_size_counter, self.snake1.snake_size_counter, self.snake0.score_counter, self.snake1.score_counter, self.snake0.is_alive,  self.snake1.is_alive, self.is_col = self.sendCollision(signal)

    def get_update(self):
        player_id = self.recv_int()
        self.recv_board()

    def take_turn(self):
        self.wait = True
        while self.wait:
            pass
    
    def click_enable(self):
        self.wait = True
        self.label.configure(text="Your turn")
        while self.wait:
            for i in range(25):       
                self.grids[i].configure(state = tkinter.NORMAL)               
                if self.board[i] == 65535:
                    self.grids[i].configure(bg = 'red')
                    self.grids[i].configure(state = tkinter.DISABLED)
            self.screen.update()

    def start_game(self):
        """
        The heart of the game : the 'while True' animation loop
        Keeps moving the snake and updating the canvas.
        Checks for events like hit food/self/other snake/wall to keep updating snake size/scores, etc
        until game is over
        """
        #
        #receive intial state of the game
        server_addr, self.s = self.open_socket('localhost', 2301)
        try:
            self.s.connect(server_addr)
            print("Connected to {:s}".format(repr(server_addr)))
            print("")
            self.id = self.recv_int()

            #wait for the start signal
            #msg = self.s.recv(4)
            #print("Let's the game begin!!!")
            msg = self.recv_msg()
            if msg == "HLD":
                print("Waiting for other player")   
            while msg != "SRT":
                msg = self.recv_msg()
                if msg == "HLD":
                    print("Waiting for other player")   

            print("Game on!\n")
            print("Your are {}s\n".format(self.id))
            self.screen.title("Player " + str(self.id))
            #self.starter_message()
            # The first food is placed outside the loop to kick start the game
            # Animation Loop
            self.recv_board()
            self.make_grid()
            self.draw_board()
            def on_closing():
                global running
                running = False
                self.screen.destroy()

            self.screen.protocol("WM_DELETE_WINDOW", on_closing)
            running = True
            while running:
                msg = self.recv_msg()
                if msg == "TRN":
                    print("Your move...\n")
                    time.sleep(1)
                    self.click_enable()
                elif msg == "INV":
                    print("That position has already been played. Try again.\n")
                elif msg == "CNT":
                    num_players = self.recv_int
                    print("There are currently %d active players.\n", num_players)
                elif msg == "UPD":
                    self.get_update()
                    self.draw_board()
                elif msg == "WAT":
                    print("Waiting for other players move...\n")
                    self.label.configure(text="Wait for other turn,...")
                    self.draw_board()
                elif msg == "WIN":
                    print("You win!\n")
                    self.label.configure(text="You win")
                    self.draw_board()
                    break
                elif msg == "LSE":
                    print("You lose.\n")
                    self.label.configure(text="You lost")
                    self.draw_board()
                    break
                elif msg == "DRW":
                    print("Draw.\n")
                    break
                else:
                    print("Unknown message")
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
