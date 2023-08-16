import secrets
import string

def generate_api_key(length):
    characters = string.ascii_letters + string.digits
    api_key = ''.join(secrets.choice(characters) for _ in range(length))
    return api_key

api_key_length = 32  # You can adjust the length as needed
random_api_key = generate_api_key(api_key_length)
print("Random API Key:", random_api_key)