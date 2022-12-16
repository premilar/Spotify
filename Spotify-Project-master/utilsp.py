
# shows a user's playlists (need to be authenticated via oauth)

from __future__ import print_function
import os
import oauth2sp as oauth2
import spotipy
import webbrowser

def prompt_for_user_token(username, scope=None, client_id = None,
        client_secret = None, redirect_uri = None):
    ''' 
        prompts the user to login if necessary and returns
        the user token suitable for use with the spotipy.Spotify 
        constructor

        Parameters:

         - username - the Spotify username
         - scope - the desired scope of the request
         - client_id - the client id of your app
         - client_secret - the client secret of your app
         - redirect_uri - the redirect URI of your app
    '''

    token_info = None
    #print ("UTIL:", client_id)
    if not client_id or not username:
#        print ("UTIL:", username)
        return None, None
    sp_oauth = oauth2.SpotifyOAuth(client_id, client_secret, redirect_uri, 
           scope=scope, cache_path="/tmp/TokenCachesp")

    # try to get a valid token for this user, from the cache,
    # if not in the cache, the create a new (this will send
    # the user to a web page where they can authorize this app)

    token_info = sp_oauth.get_cached_token(username)

    auth_url = None
    if not token_info:
        auth_url = sp_oauth.get_authorize_url()

    # Auth'ed API request
    if token_info:
        return token_info['access_token'], auth_url
    else:
        return None, auth_url

def get_access_token (username=None, scope=None, client_id = None,
        client_secret = None, redirect_uri = None, response = None):
#    print ("UTIL: get_access_token")

    sp_oauth = oauth2.SpotifyOAuth(client_id, client_secret, redirect_uri, 
        scope=scope, cache_path="/tmp/TokenCachesp")

    code = sp_oauth.parse_response_code(response)
    token_info = sp_oauth.get_access_token(code)

    # Auth'ed API request
    if token_info:
        return token_info['access_token']
    else:
        return None
