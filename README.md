# This is project of Network Programing

## Game Center sequence

```mermaid
sequenceDiagram
autonumber

participant user as User
participant gamecenter as Game Center
participant server as Game Server
participant tcp as TCP Socket

user->>+gamecenter: Open game center
gamecenter->>+server:  Login with username and password
server->>+gamecenter:  Validate and Return Response
gamecenter->>+server:  Validate successfully
server->>+gamecenter:  Show Game Menu for player to choose
gamecenter->>+server: Choose one game and play it
server->>+gamecenter: Handle request and return response for player

gamecenter->>+tcp: Choose chat function in Menu
tcp->>+server: Handle buffer
server->>+gamecenter: Return messages to the respective players
```
