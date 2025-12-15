You are analyzing the gRPC and protobuf layer.
## Strong rule
- 서술어 사용하지 말 것.
- 한국어로 작성 
## Tasks:
1. Extract all service and message definitions from the .proto file.
2. For each RPC:
   - Describe its purpose.
   - List its Request and Response fields.
3. From the generated C++ code:
   - Show how the client stub is constructed.
   - Identify how each RPC is invoked.
4. Connect these to the client code:
   - Where the stub is created.
   - Where each request is constructed.
   - Where each response is processed.

## Output:
- Services summary
- RPC details
- C++ stub mapping
- Integration points in gRPCFileClient
