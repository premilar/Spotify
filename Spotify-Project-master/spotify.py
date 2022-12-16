import sys
import spotipy
#import spotipy.util as util
import utilsp_11 as util
import json
#import client

SPOTIPY_CLIENT_ID='b64763c5b9334249a91d9e9d3afd3eb2'
SPOTIPY_CLIENT_SECRET='6dc87ad42c0341889b6e8f4e0dad5cba'
SPOTIPY_REDIRECT_URI='http://iesc-s2.mit.edu/6S08dev/simmyp/final/sb1.py'
#SPOTIPY_REDIRECT_URI='https://stage.cooldimi.net:9000'

username = "s1mmyp"

scope = 'user-library-read playlist-modify-public user-follow-read user-follow-modify user-top-read'
token, auth_url = util.prompt_for_user_token(username, scope, SPOTIPY_CLIENT_ID, SPOTIPY_CLIENT_SECRET, SPOTIPY_REDIRECT_URI)
#token = util.prompt_for_user_token('s1mtest', scope)

if token:
    print("Token found!")
    sp = spotipy.Spotify(auth=token)
    results = sp.current_user_playlists()
    #print(results)
    numPlaylists = results['total']
#    name = results['items'][0]['name']
#    playlist_id = results['items'][0]['id']
#    numTracks = results['items'][0]['tracks']['total']
#    print(results)
    #print("Number of playlists: " + str(numPlaylists))
#    print("Name of playlist: " + name)
#    print("Playlist ID: " + playlist_id)
#    print("Number of Tracks in Playlist: " + str(numTracks))
    for item in results['items']:
        numTracks = item['tracks']['total']
        playlist_id = item['id']
        name = item['name']
#        print("Name of playlist: " + name)
#        print("Playlist ID: " + playlist_id)
#        print("Number of tracks: " + str(numTracks))

    results_2 = sp.user_playlist(username, playlist_id)
    #print(results_2['tracks']['items'][0])
    for item in results_2['tracks']['items']:
        track_id = item['track']['id']
        artist = item['track']['artists'][0]['name']
        name = item['track']['name']
        #print(track_id)
        #print(artist)
        #print(name)
#        name = item['name']
#        print("Name of playlist: " + name)
#        print("Track ID: " + track_id)

    numTracks = results_2['tracks']['total']
    #print(numTracks)

    #print("Number of tracks: " + str(numTracks))

#    results_3 = sp.user_playlist_tracks(username, playlist_id)
#    for item in results['items']:
#        print (item)
    #print(results_3)

    print(sp.user_playlist_create(username, "My Playlist", True)['id'])
    #print(playlist_id)
    #print(track_id)
    #print(sp.track("5421318643512hd45"))
    #my_id = [sp.current_user_top_tracks()['items'][0]['id']]
    #print(my_id)
    #sp.user_playlist_add_tracks(username, playlist_id, my_id)
    #sp.user_playlist_add_tracks(username, playlist_id, my_id)


else:
    print ("Can't get token for", username)

