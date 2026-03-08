import re

with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'r') as f:
    content = f.read()

# Remove the ResponseStream and GetStream and wait for response code
# Actually we still need WaitForResponse and OnRPCTransferDone for StartJcm request
content = re.sub(r'static void GenerateReplyWork\(intptr_t arg\)\s*\{.*?\}\s*\}', '', content, flags=re.DOTALL)
content = re.sub(r'void OnGetStreamOnNext\(const RequestOptions & requestOptions\)\s*\{.*?\}', '', content, flags=re.DOTALL)
content = re.sub(r'void OnGetStreamOnDone\(::pw::Status status\)\s*\{.*?\}', '', content, flags=re.DOTALL)

with open('examples/jf-control-app/commands/pairing/PairingCommand.cpp', 'w') as f:
    f.write(content)
