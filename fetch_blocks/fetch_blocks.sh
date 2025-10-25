#!/bin/bash
X=${1:-5}
OUTPUT_DIR="./blocks"
mkdir -p "$OUTPUT_DIR"
rm -rf "$OUTPUT_DIR"/*
LATEST_HASH=$(curl -s https://api.blockcypher.com/v1/btc/main | grep "\"hash\""|awk -F \" '{printf "%s",$4}')
echo "Latest block hash: $LATEST_HASH"

for ((i=0; i<"$X"; i++)); do
echo "Fetching block $((i+1))..."
BLOCK_DATA=$(curl -s "https://api.blockcypher.com/v1/btc/main/blocks/$LATEST_HASH")

HASH=$(echo "$BLOCK_DATA" | grep "\"hash\""|awk -F \" '{printf "%s",$4}')
HEIGHT=$(echo "$BLOCK_DATA" | grep "\"height\""|awk -F \" '{printf "%s",$3}'|sed 's/[:," "]//g')
TOTAL=$(echo "$BLOCK_DATA" | grep "\"total\""|awk -F \" '{printf "%s",$3}'|sed 's/[:," "]//g')
TIME=$(echo "$BLOCK_DATA" | grep "\"time\""|awk -F \" '{printf "%s",$4}')
RELAYED_BY=$(echo "$BLOCK_DATA" | grep "\"relayed_by\""|awk -F \" '{printf "%s",$4}')
PREV_BLOCK=$(echo "$BLOCK_DATA" | grep "\"prev_block\""|awk -F \" '{printf "%s",$4}')

FILE="$OUTPUT_DIR/block_$HEIGHT.txt"
echo "Hash: $HASH">$FILE
echo "Height: $HEIGHT">>$FILE
echo "Total: $TOTAL">>$FILE
echo "Time: $TIME">>$FILE
echo "Relayed by: $RELAYED_BY">>$FILE
echo "Previous Block: $PREV_BLOCK">>$FILE
echo "Saved to $FILE"
LATEST_HASH="$PREV_BLOCK"
done
