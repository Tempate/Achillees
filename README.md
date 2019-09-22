# Achillees

This project is an attempt at making a readable and accessible chess engine. It's written in plain C and it currently uses the GPLv3+ license. The command `help` will show you the list of human-friendly commands. You can also set it to UCI mode with the command `uci`.

All comments and feedback are appreciated. Feel free to join us at `##chessprogramming` on [Freenode IRC](https://webchat.freenode.net) and share your thoughts.

You are also free to challenge it at [Lichess](https://lichess.org/@/Achillees).

## Features

> The project is under active development and new features will come shortly.

### Move generation
- Plain magic bitboards
- Legal move generation

### Evaluation
- Material count
- Piece-Square tables

### Search
- AlphaBeta Pruning
- Move ordering
- Transposition Table
- Time management
- Quiescence search
- Check extensions
- Late move reduction
- Aspiration windows
- Null window search
- Internal Iterative Deepening

- Pruning
  - Razoring
  - Delta pruning
  - Null move pruning
  - Futility pruning
  - Reverse futility pruning
  - Late move pruning

## Running the code

You can get the code running by following three simple steps:

1. Download the code
```
git clone https://github.com/Tempate/Achillees
```

2. Type the command:
```
make release
```

3. An executable should now appear inside your bin folder, you can run it by doing: 
```
./Achillees
```

## Credits

Special thanks to the [chessprogramming wiki](https://www.chessprogramming.org/Main_Page), as most of the knowledge needed came from reading through it; to [ROCE](http://www.rocechess.ch/rocee.html), without which it would have taken ages to pass perft; and to [Daily Chess](https://www.dailychess.com/rival/programming/index.php) for their fantastic guide.

I also feel the need to thank everyone on `##chessprogramming` on [Freenode IRC](https://webchat.freenode.net) for their support, encouragement, and exhaustive help.
