
GENIE=tools/genie
mkdir -p tools

if [ ! -f "$PWD/$GENIE" ]; then
	wget https://github.com/bkaradzic/bx/raw/master/tools/bin/darwin/genie -P tools
	chmod +x $GENIE
fi

echo "Setup complete"
