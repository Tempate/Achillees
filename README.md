# Achillees

This project is an attempt at making a readable and accessible chess engine. It's written in plain C and it currently uses the GCC license.

All comments and feedback are appreciated. Feel free to join us at `##chessprogramming` on [Freenode IRC](https://webchat.freenode.net) and share your thoughts.

You are also free to challenge it on lichess.org: https://lichess.org/@/Achillees

## Features

### Evaluation
- Material count
- Pawn structure
- Pair of bishops
- Rooks on open files, on the sevent rank, and on the same file
- King safety
- Piece-Square tables

### Search
- AlphaBeta Pruning

> Note that the project is under active development and new features will come shortly.

## Running the code

You can get the code running by following three simple steps:

1. Download the code
```
git clone https://github.com/Tempate/Achillees
```

2. Get into the Release directory and run the command:
```
make all
```

3. An executable should now appear inside your Release folder, you can run it by doing `./Achillees`

## Credits

Special thanks to the [chessprogramming wiki](), as most of the knowledge needed came from reading through it, to [ROCE](http://www.rocechess.ch/rocee.html), without which it would have taken ages to pass perft, and to [Daily Chess](https://www.dailychess.com/rival/programming/index.php) for there fantastic guide.

I also feel the need to thank everyone on `##chessprogramming` on [Freenode IRC](https://webchat.freenode.net) for their support, encouragement, and help.
