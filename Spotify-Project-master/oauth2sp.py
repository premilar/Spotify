from __future__ import print_function
import base64
import requests
import os
import json
import time
import sys
import spotipy

INFO = False

# Workaround to support both python 2 & 3
try:
    import urllib.request, urllib.error
    import urllib.parse as urllibparse
except ImportError:
    import urllib as urllibparse



class SpotifyOauthError(Exception):
    pass


class SpotifyClientCredentials(object):
    OAUTH_TOKEN_URL = 'https://accounts.spotify.com/api/token'

    def __init__(self, client_id=None, client_secret=None, proxies=None):
        #print ("SpotifyClientCredentials")

        if not client_id:
            raise SpotifyOauthError('No client id')

        if not client_secret:
            raise SpotifyOauthError('No client secret')

        self.client_id = client_id
        self.client_secret = client_secret
        self.tokens = {}
        self.proxies = proxies

    def get_access_token(self, username):
        #print ("SpotifyClientCredentials: get_access_token")
        """
        If a valid access token is in memory, returns it
        Else feches a new token and returns it
        """
        token_info = None
        if username in self.tokens:
            token_info = self.tokens[username]
        if token_info and not self._is_token_expired(self.token_info):
            return self.token_info['access_token']

        token_info = self._request_access_token()
        token_info = self._add_custom_values_to_token_info(token_info)
        self.token[username] = token_info
        return token_info['access_token']

    def _request_access_token(self):
        #print ("SpotifyClientCredentials: request_access_token")
        """
        Gets client credentials access token 
        """
        payload = { 'grant_type': 'client_credentials'}

        if sys.version_info[0] >= 3: # Python 3
            auth_header = base64.b64encode(str(self.client_id + ':' + self.client_secret).encode())
            headers = {'Authorization': 'Basic %s' % auth_header.decode()}
        else: # Python 2
            auth_header = base64.b64encode(self.client_id + ':' + self.client_secret)
            headers = {'Authorization': 'Basic %s' % auth_header}

        response = requests.post(self.OAUTH_TOKEN_URL, data=payload,
            headers=headers, verify=True, proxies=self.proxies)
        if response.status_code is not 200:
            raise SpotifyOauthError(response.reason)
        token_info = response.json()
        return token_info

    def _is_token_expired(self, token_info):
        self._info ("SpotifyClientCredentials: _is_token_expired")
        now = int(time.time())
        return token_info['expires_at'] - now < 60

    def _add_custom_values_to_token_info(self, token_info):
        self._info ("SpotifyClientCredentials: _add_custom_values_to_token_info")
        """
        Store some values that aren't directly provided by a Web API
        response.
        """
        token_info['expires_at'] = int(time.time()) + token_info['expires_in']
        return token_info


class SpotifyOAuth(object):
    '''
    Implements Authorization Code Flow for Spotify's OAuth implementation.
    '''

    OAUTH_AUTHORIZE_URL = 'https://accounts.spotify.com/authorize'
    OAUTH_TOKEN_URL = 'https://accounts.spotify.com/api/token'
    OAUTH_ME_URL = 'https://api.spotify.com/v1/me/'

    def __init__(self, client_id, client_secret, redirect_uri,
            state=None, scope=None, cache_path=None, proxies=None):
        self._info ("SpotifyOAuth")
        '''
            Creates a SpotifyOAuth object

            Parameters:
                 - client_id - the client id of your app
                 - client_secret - the client secret of your app
                 - redirect_uri - the redirect URI of your app
                 - state - security state
                 - scope - the desired scope of the request
                 - cache_path - path to location to save tokens
        '''

        self.client_id = client_id
        self.client_secret = client_secret
        self.redirect_uri = redirect_uri
        self.state=state
        self.cache_path = cache_path
        self.scope=self._normalize_scope(scope)
        self.proxies = proxies
        self.tokens = self.get_cached_token()#{}
        if self.tokens is None:
            self.tokens = {}
        self._info ("__INIT__: %s" % self.tokens)

    def get_cached_token(self, username=None):
        self._info ("SpotifyOAuth: get_cached_token")
        ''' Gets a cached auth token
        '''
        #print ("username:", username)
        token_info = None
        if self.cache_path:
            try:
                f = open(self.cache_path)
                token_string = f.read()
                f.close()
                tokens = json.loads(token_string)
                #print ("TOKENS: ", tokens)
                
                if not username:
                   return tokens
    
                if username not in tokens:
                    return None

                token_info = tokens[username]
                #print ("oauth2:", token_info)
                #print()
                #print ("CACHED:", token_info)
                # if scopes don't match, then bail
                if 'scope' not in token_info or not self._is_scope_subset(self.scope, token_info['scope']):
                    return None

                if self._is_token_expired(token_info):
                     token_info = self.refresh_access_token(token_info['refresh_token'])
                     self.tokens[username] = token_info

            except IOError:
                pass
        return token_info

    def _save_token_info(self, tokens):
        #print ("SpotifyOAuth: _save_token_info")
        if self.cache_path:
            try:
                f = open(self.cache_path, 'w')
                f.write(json.dumps(tokens))
                f.close()
            except IOError:
                self._warn("couldn't write token cache to " + self.cache_path)
                pass

    def _is_scope_subset(self, needle_scope, haystack_scope):
        #print ("SpotifyOAuth: _is_scope_subset")
        needle_scope = set(needle_scope.split())
        haystack_scope = set(haystack_scope.split())

        return needle_scope <= haystack_scope

    def _is_token_expired(self, token_info):
        #print ("SpotifyOAuth: _is_token_expired")
        now = int(time.time())
        return token_info['expires_at'] < now

    def get_authorize_url(self):
        #print ("SpotifyOAuth: get_authorize_url")
        """ Gets the URL to use to authorize this app
        """
        payload = {'client_id': self.client_id,
                   'response_type': 'code',
                   'redirect_uri': self.redirect_uri}
        if self.scope:
            payload['scope'] = self.scope
        if self.state:
            payload['state'] = self.state

        urlparams = urllibparse.urlencode(payload)

        return "%s?%s" % (self.OAUTH_AUTHORIZE_URL, urlparams)

    def parse_response_code(self, url):
        #print ("SpotifyOAuth: parse_response_code")
        """ Parse the response code in the given response url

            Parameters:
                - url - the response url
        """

        try:
            return url.split("?code=")[1].split("&")[0]
        except IndexError:
            return None

    def get_access_token(self, code):
        #print ("SpotifyOAuth: get_access_token")
        """ Gets the access token for the app given the code

            Parameters:
                - code - the response code
        """

        payload = {'redirect_uri': self.redirect_uri,
                   'code': code,
                   'grant_type': 'authorization_code'}
        if self.scope:
            payload['scope'] = self.scope
        if self.state:
            payload['state'] = self.state

        if sys.version_info[0] >= 3: # Python 3
            auth_header = base64.b64encode(str(self.client_id + ':' + self.client_secret).encode())
            headers = {'Authorization': 'Basic %s' % auth_header.decode()}
        else: # Python 2
            auth_header = base64.b64encode(self.client_id + ':' + self.client_secret)
            headers = {'Authorization': 'Basic %s' % auth_header}

        response = requests.post(self.OAUTH_TOKEN_URL, data=payload,
            headers=headers, verify=True, proxies=self.proxies)
        if response.status_code is not 200:
            raise SpotifyOauthError(response.reason)


        token_info = response.json()
        token_info = self._add_custom_values_to_token_info(token_info)

        sp = spotipy.Spotify(auth=token_info['access_token'])
        username = sp.me()['id']
        #print ("Usrename: ", username, token_info)
        #print ("GET_ACCESS:", self.tokens)
        self.tokens[username] = token_info
        self._save_token_info(self.tokens)

        #headers = {'Authorization': 'Bearer %s' % token_info['access_token']}
        #headers['Content-Type'] = 'application/json'
        #print ("headers:", headers)
        #response = requests.post("https://api.spotify.com/v1/me",
        #    headers=headers)
        #print ("response", response)

        return token_info

    def _normalize_scope(self, scope):
        #print ("SpotifyOAuth: _normalize_scope")
        if scope:
            scopes = scope.split()
            scopes.sort()
            return ' '.join(scopes)
        else:
            return None

    def refresh_access_token(self, refresh_token):
        #print ("SpotifyOAuth: refresh_access_token")
        payload = { 'refresh_token': refresh_token,
                   'grant_type': 'refresh_token'}

        if sys.version_info[0] >= 3: # Python 3
            auth_header = base64.b64encode(str(self.client_id + ':' + self.client_secret).encode())
            headers = {'Authorization': 'Basic %s' % auth_header.decode()}
        else: # Python 2
            auth_header = base64.b64encode(self.client_id + ':' + self.client_secret)
            headers = {'Authorization': 'Basic %s' % auth_header}

        response = requests.post(self.OAUTH_TOKEN_URL, data=payload,
            headers=headers, proxies=self.proxies)
        if response.status_code != 200:
            if False:  # debugging code
                #print('headers', headers)
                #print('request', response.url)
                pass
            self._warn("couldn't refresh token: code:%d reason:%s" \
                % (response.status_code, response.reason))
            return None
        token_info = response.json()
        token_info = self._add_custom_values_to_token_info(token_info)
        if not 'refresh_token' in token_info:
            token_info['refresh_token'] = refresh_token
        #self._save_token_info(token_info)
        return token_info

    def _add_custom_values_to_token_info(self, token_info):
        #print ("SpotifyOAuth: _add_custom_values_to_token_info")
        '''
        Store some values that aren't directly provided by a Web API
        response.
        '''
        token_info['expires_at'] = int(time.time()) + token_info['expires_in']
        token_info['scope'] = self.scope
        return token_info

    def _warn(self, msg):
        print('warning:' + msg, file=sys.stderr)

    def _info (self, msg):
        if INFO:
            print('Info:' + msg, file=sys.stdout)

