import sys

with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'r') as f:
    lines = f.readlines()

new_lines = []
skip = False

for i, line in enumerate(lines):
    if line.startswith('struct RequestOptionsContext'):
        skip = True
    
    if line.startswith('static void GenerateReplyWork'):
        skip = True
        
    if line.startswith('void OnGetStreamOnNext'):
        skip = True
        
    if line.startswith('void OnGetStreamOnDone'):
        skip = True

    if not skip:
        new_lines.append(line)
        
    if skip and line.startswith('}'):
        # Check if the block has closed
        pass
        
