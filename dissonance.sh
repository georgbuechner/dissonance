export TERMINFO=$(whereis terminfo | awk '{print $2}')
dissonance_bin $1 $2 $3 $4
