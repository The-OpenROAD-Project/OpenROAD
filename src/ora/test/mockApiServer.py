from http.server import BaseHTTPRequestHandler, HTTPServer
import json

class MockHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8')
        payload = json.loads(body)

        query = payload.get("query", "")
        list_sources = payload.get("list_sources", False)

        response_data = {
            "response": f"Sent Message: {query}",
        }

        if list_sources:
            response_data["sources"] = ["source_1", "source_2", "source_3"]

        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response_data).encode('utf-8'))

def start_server():
    port = 8080
    while True:
        try:
            server = HTTPServer(("0.0.0.0", port), MockHandler)
            print(f"Mock API Server started at port {port}")
            server.serve_forever()
        except Exception as e:
            print(f"Failed to start server at port {port}")
            print(e)

if __name__ == "__main__":
    start_server()
