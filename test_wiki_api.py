import urllib.request
import json

def get_wiki_image(title):
    # Wikimedia Commons API
    url = f"https://en.wikipedia.org/w/api.php?action=query&titles={title}&prop=pageimages&format=json&pithumbsize=800"
    req = urllib.request.Request(url, headers={'User-Agent': 'Arduino-Bot/1.0'})
    try:
        response = urllib.request.urlopen(req)
        data = json.loads(response.read())
        pages = data['query']['pages']
        for page_id, page_info in pages.items():
            if 'thumbnail' in page_info:
                return page_info['thumbnail']['source']
    except Exception as e:
        pass
    return None

print("Arduino Uno:", get_wiki_image("Arduino_Uno"))
print("HC-SR04:", get_wiki_image("Sonar")) # Example
