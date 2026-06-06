from duckduckgo_search import DDGS

try:
    results = DDGS().images("Arduino Uno R3 board", max_results=2)
    for r in results:
        print(r['image'])
except Exception as e:
    print(f"Error: {e}")
