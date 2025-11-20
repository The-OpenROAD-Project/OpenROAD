from fastmcp import FastMCP

# Create an MCP server
mcp = FastMCP("OpenROAD")

# Add an addition tool
@mcp.tool()
def add(x: int, y: int) -> int:
    """Add two numbers"""
    return x + y

if __name__ == "__main__":
    # Run the server
    mcp.run(transport='stdio')
