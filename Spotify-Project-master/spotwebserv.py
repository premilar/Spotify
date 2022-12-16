import sys
import spotipy
#import spotipy.util as util
import utilsp as util
import json
#import client
import records
import requests
from datetime import datetime

PLAYLIST_DATABASE = "playlists"
LOCATIONS_DATABASE = "user_tracking"

db = records.Database('mysql://student:6s08student@localhost/iesc')

SPOTIPY_CLIENT_ID='b64763c5b9334249a91d9e9d3afd3eb2'
SPOTIPY_CLIENT_SECRET='6dc87ad42c0341889b6e8f4e0dad5cba'
SPOTIPY_REDIRECT_URI='http://iesc-s2.mit.edu/6S08dev/simmyp/final/sb1.py'
scope = 'user-library-read playlist-modify-public user-follow-read user-follow-modify user-top-read'
sep = "&&"

locations={
    "Student Center":[(-71.095863,42.357307),(-71.097730,42.359075),(-71.095102,42.360295),(-71.093900,42.359340),(-71.093289,42.358306)],
    "Dorm Row":[(-71.093117,42.358147),(-71.092559,42.357069),(-71.102987,42.353866),(-71.106292,42.353517)],
    "Simmons/Briggs":[(-71.097859,42.359035),(-71.095928,42.357243),(-71.106356,42.353580),(-71.108159,42.354468)],
    "Boston FSILG (West)":[(-71.124664,42.353342),(-71.125737,42.344906),(-71.092478,42.348014),(-71.092607,42.350266)],
    "Boston FSILG (East)":[(-71.092409,42.351392),(-71.090842,42.343589),(-71.080478,42.350900),(-71.081766,42.353771)],
    "Stata/North Court":[(-71.091636,42.361802),(-71.090950,42.360811),(-71.088353,42.361112),(-71.088267,42.362476),(-71.089769,42.362618)],
    "East Campus":[(-71.089426,42.358306),(-71.090885,42.360716),(-71.088310,42.361017),(-71.087130,42.359162)],
    "Vassar Academic Buildings":[(-71.094973,42.360359),(-71.091776,42.361770),(-71.090928,42.360636),(-71.094040,42.359574)],
    "Infinite Corridor/Killian":[(-71.093932,42.359542),(-71.092259,42.357180),(-71.089619,42.358274),(-71.090928,42.360541)],
    "Kendall Square":[(-71.088117,42.364188),(-71.088225,42.361112),(-71.082774,42.362032)],
    "Sloan/Media Lab":[(-71.088203,42.361017),(-71.087044,42.359178),(-71.080071,42.361619),(-71.082796,42.361905)],
    "North Campus":[(-71.11022,42.355325),(-71.101280,42.363934),(-71.089950,42.362666),(-71.108361,42.354484)],
    "Technology Square":[(-71.093610,42.363157),(-71.092130,42.365837),(-71.088182,42.364188),(-71.088267,42.362650)]
}


# returns area lat, lon pair falls into if any
def within_area(point_coord,poly):
    new_list=[]
    new_list.append(0-point_coord[0])
    new_list.append(0-point_coord[1])
    for i in range(len(poly)):
        poly[i]=(poly[i][0]+new_list[0],
                 poly[i][1]+new_list[1])
    point_coord=(0,0)
    edge_num=0
    point=0
    for i in range(len(poly)):
        poly.append(poly[0])
        if poly[i][1]>0 and poly[i+1][1]<0 or poly[i][1]<0 and poly[i+1][1]>0:
            if poly[i][0]>0 and poly[i+1][0]>0:
                edge_num+=1
            elif poly[i][0]/abs(poly[i][0])!=poly[i+1][0]/abs(poly[i+1][0]):
                point= ((poly[i][0]*poly[i+1][1])-(poly[i][1]*poly[i+1][0]))/ (poly[i+1][1]-poly[i][1])
                if point>0:
                    edge_num+=1
        else:
            pass
    if edge_num%2==0:
        return False
    return True

def get_area(point_coord,locations):
    for i in locations:
        if within_area(point_coord,locations[i]):
            return i
    return "Outside MIT"

def sign(x):
    if x > 0:
        return 1
    elif x == 0:
        return 0
    else:
        return -1


def web_handler(request):
    method = request['REQUEST_METHOD']

    if method == 'GET':
        outpt = ""
        parameters = request['GET']



        #Gets playlist of a user given username
        if "userID" in parameters and "playlistID" not in parameters:
            userID = parameters["userID"]
            token, auth_url = util.prompt_for_user_token(userID, scope, SPOTIPY_CLIENT_ID, SPOTIPY_CLIENT_SECRET, SPOTIPY_REDIRECT_URI)
            sp = spotipy.Spotify(auth=token)
            #if token:
                #return token
                #return "GOT TOKEN!"
            results = sp.current_user_playlists()
            for item in results['items']:
                    try:
                        name = item['name']
                        playlist_id = item['id']
                        outpt += (name + ":" + playlist_id + ",")
                    except KeyError:
                        outpt += "None"
            numPlaylists = results['total']
            outpt += sep + str(numPlaylists)
            
        #Gets personal user's playlists given username and creates a new playlist if so requested
        elif "selfuserID" in parameters and "nw" in parameters:
            userID = parameters["selfuserID"]
            token, auth_url = util.prompt_for_user_token(userID, scope, SPOTIPY_CLIENT_ID, SPOTIPY_CLIENT_SECRET, SPOTIPY_REDIRECT_URI)
            sp = spotipy.Spotify(auth=token)
            #if token:
                #return token
                #return "GOT TOKEN!"
            nw = parameters["nw"]
            if nw == "True":
                sp.user_playlist_create(userID, "My Playlist", True)
                return "New playlist will be created"
            else:
                results = sp.current_user_playlists()
                #return sp.current_user_playlists()
                #return sp.user()
                #return sp.user_playlist(userID)

                for item in results['items']:
                    try:
                        name = item['name']
                        playlist_id = item['id']
                        outpt += (name + ":" + playlist_id + ",")
                    except KeyError:
                        outpt += "None"
                numPlaylists = results['total']
                outpt += sep + str(numPlaylists)
                
        #Gets songs in a playlist
        elif "userID" in parameters and "playlistID" in parameters and "songID" not in parameters:
            userID = parameters["userID"]
            token, auth_url = util.prompt_for_user_token(userID, scope, SPOTIPY_CLIENT_ID, SPOTIPY_CLIENT_SECRET, SPOTIPY_REDIRECT_URI)
            sp = spotipy.Spotify(auth=token)
            playlistID = parameters['playlistID']
            results = sp.user_playlist(userID, playlistID)
            #return results
            for item in results['tracks']['items']:
                track_id = item['track']['id']
                name = item['track']['name']
                outpt += name + ":" + track_id + ","
            numTracks = results['tracks']['total']
            outpt += sep + str(numTracks)


        ### INCLUDE A SECTION FOR SONG INFO GIVEN SONG ID
        elif "songID" in parameters:
            songID = parameters["songID"]
            r = requests.get("https://api.spotify.com/v1/tracks/"+songID)
            outpt+="Song Name: "+r.json()["name"]+"##Artist(s): "
            for i in range(len(r.json()["album"]["artists"])):
                outpt+=r.json()["album"]["artists"][i]["name"]
                if(i!=len(r.json()["album"]["artists"])-1):
                    outpt+=','
            outpt+="##Album: "+(r.json()["album"]["name"])



        #Claire's diversity home data 

        elif "selfuserID" in parameters and "lon" not in parameters:
            selfuserID = parameters["selfuserID"]
            query = "SELECT * FROM playlists WHERE username=:username ORDER BY ID DESC"
            user = db.query(query,username=selfuserID)
            userdict=user.as_dict()
            songlist = [] #songlist is a list of dictionaries containing each song json
            for item in userdict:
                if item['username']==selfuserID:
                    r= requests.get("https://api.spotify.com/v1/tracks/"+item["track_id"])
                    songlist.append(r.json())
            diversity = diversity_track(selfuserID,songlist)  
            diversitylist = diversity_plot(selfuserID,songlist)
            newdiversitylist = resample(diversitylist,30)
            outpt += "Diversity##"+str(diversity) +"NUM:"
            for i in range(0,30):
                outpt+=str(i*4+12)+","+str(round(newdiversitylist[i],2))
                if i!=29:
                    outpt+=','

        #GROUP MODE HOME
        elif "lat" in parameters and "lon" in parameters and "selfuserID" in parameters:
            lat = float(parameters["lat"])
            lon = float(parameters["lon"])
            selfuserID = parameters["selfuserID"]
            area = get_area((lon, lat), locations)
            query = "SELECT * FROM playlists WHERE location=:location ORDER BY LIKES DESC"
            songs = db.query(query,location=area)
            songlist = songs.as_dict()
            #print(songlist)
            userlist = []
            usercount= 0
            songcount=0
            for item in songlist:
                #print(item)
                # if("\\r\\n" in item["track_id"]):
                #     item["track_id"]=item["track_id"][:-4]
                track = item["track_id"]
                #print("https://api.spotify.com/v1/tracks/"+track)
                #print(track)
                if "5ELR" not in item["track_id"]:
                    songcount+=1
                    r= requests.get("https://api.spotify.com/v1/tracks/"+track)
                    #print(r.json())
                    outpt+=r.json()["name"] + ":" + track+"##"+str(item["likes"]) + ","
                    user = False
                    for name in userlist:
                        if item['username']==name:
                            user = True
                    if user== False:
                        userlist.append(item['username'])
                        usercount+=1
            outpt+="&&"+str(songcount) +"##"
            outpt+="Users Posting: "+str(usercount) +"##"
            outpt+="Area: " + area

        return outpt


    elif method == 'POST':
        parameters = request['POST']
        outpt = ""
        if ("playlistID" in parameters and "nw" in parameters and "group_mode" in parameters and "like" in parameters and "selfuserID" in parameters and "songID" in parameters and "lat" in parameters and "lon" in parameters):
            userID = parameters["selfuserID"]
            token, auth_url = util.prompt_for_user_token(userID, scope, SPOTIPY_CLIENT_ID, SPOTIPY_CLIENT_SECRET, SPOTIPY_REDIRECT_URI)
            sp = spotipy.Spotify(auth=token)
            playlistID = parameters["playlistID"]
            
            songID = [parameters["songID"]]
            if parameters["group_mode"]=="False":
                lat = float(parameters["lat"])
                lon = float(parameters["lon"])
                area = get_area((lon, lat), locations)
                like = "1"
                #like = parameters["likes"]
                currenttime = datetime.now().isoformat()
                sp.user_playlist_add_tracks(userID, playlistID, songID)
                query = "INSERT INTO " + PLAYLIST_DATABASE + " (time,playlist_id,username,track_id,location,likes) VALUES ('"+currenttime + "' , '" +playlistID+ "' , '"+ userID+"' , '" + parameters["songID"] + "' , '" +area+ "' , '"+like+"')"
                db.query(query)
                outpt += "Added to user playlist and database"
            else:
                sp.user_playlist_add_tracks(userID, playlistID, songID)
                #add to old playlist
        elif("selfuserID" in parameters and "group_mode" in parameters and "nw" in parameters and "like" in parameters and "songID" in parameters and "lat" in parameters and "lon" in parameters ):
            userID = parameters["selfuserID"]
            token, auth_url = util.prompt_for_user_token(userID, scope, SPOTIPY_CLIENT_ID, SPOTIPY_CLIENT_SECRET, SPOTIPY_REDIRECT_URI)
            sp = spotipy.Spotify(auth=token)
            songIDlist = [parameters["songID"]]
            songID = parameters["songID"]
            if parameters["group_mode"]=="False": #you're in individual mode so you want to add to a new playlist
                #add to new playlist
                lat = float(parameters["lat"])
                lon = float(parameters["lon"])
                area = get_area((lon, lat), locations)
                like = "1"
                ##like = parameters["likes"]
                currenttime = datetime.now().isoformat()
                create = sp.user_playlist_create(userID,"New_playlist",True)
                playlistID = create['id']
                sp.user_playlist_add_tracks(userID, playlistID, songIDlist)
                query = "INSERT INTO " + PLAYLIST_DATABASE + " (time,username,track_id,location,likes) VALUES ('"+currenttime + "' , '"+ userID+"' , '" + songID + "' , '" +area+ "' , '"+like+"')"
                db.query(query)
                outpt += "Added to user playlist and database"
            else:
                create = sp.user_playlist_create(userID,"New_playlist",True)
                playlistID = create['id']
                sp.user_playlist_add_tracks(userID, playlistID, songID)
                #add to new playlist in group mode
        elif ("songID" in parameters and "like" in parameters and "lat" in parameters and "lon" in parameters):
            if parameters["like"]=="-1":
                like=int(parameters["like"])
                songID = parameters["songID"]
                lat = float(parameters["lat"])
                lon = float(parameters["lon"])
                area = get_area((lon, lat), locations)
                query = "SELECT * FROM playlists WHERE location=:location and track_ID=:track_ID ORDER BY ID DESC LIMIT 1" 
                row = db.query(query,location=area,track_ID=songID)
                recent_entry=row.as_dict()
                ID=int(recent_entry[0]["ID"])
                likes=int(recent_entry[0]["likes"])
                new_like=likes+like
                query="UPDATE playlists SET likes=:likes WHERE ID=:ID" 
                db.query(query,ID=ID,likes=new_like)
                outpt+="Song downvoted"
        else:
            query_result=db.query("SELECT * FROM " + DATABASE_TABLE)
            rows = query_result.as_dict()
            outpt = str(rows)
            return outpt
            #outpt += "Incorrect POST parameters"

        return outpt



#    elif method == 'POST':
#        parameters = request['POST']
#        outpt = ""
        #posting songs to add tracks to playlist and adding this track to diversity/groupcontribution playlist
#        if("playlistID" in parameters and "userID" in parameters and "songID" in parameters and "spoof" in parameters):
        #     playlistID = parameters["playlistID"]
        #     userID = parameters["userID"]
        #     songID = parameters["songID"]
        #     currenttime = datetime.now().isoformat()
        #     spoof = parameters["spoof"] #spoof should be true or false
        #     category = "Pop"
        #     if(spoof=="False"):
        #         r = requests.post('https://api.spotify.com/v1/users/'+userID+'playlists/'+playlistID+'/tracks?uris='+songID)
        #     query = "INSERT INTO " + PLAYLIST_DATABASE + " (time,playlist_id,username,track_id,category) VALUES ('"+currenttime + "' , '" +playlistID+ "' , '"+ userID+"' , '" + songID + "' , '" +category+"')"
        #     db.query(query)
        #     outpt += "Added to user playlist and database"

        # else:
        #     outpt += "Incorrect POST parameters"

        # return outpt




def diversity_track(user,song_list):
    diversity = 0
    artist_cont = 0
    artist_list = []
    for song in song_list:
        diversity += song["popularity"]
        for artist in song["artists"]:
            if artist["name"] not in artist_list:
                artist_list+= artist["name"]
            else:
                artist_cont +=1

    #algorithm involves taking into account a 0 to 100 pop avg, 0 being high diversity, and a varied playlist value of 0 to 50, 0 being high diversity.
        artist_cont*=(50/len(artist_list))
        diversity = diversity/len(song_list)

    return (150-diversity-artist_cont) #returns full 150 if high diversity now.

def diversity_plot(user,song_list):
    diversity = []
    ongoingsonglist = []
    for i in range(len(song_list)):
        ongoingsonglist.append(song_list[i])
        diversity.append(diversity_track(user,ongoingsonglist)*60/150)
    return diversity

def lcm(x, y):
   if x > y:            # Store smallest number
       temp = x
   else:
       temp = y
   while (True):    # loop
        # run through temp values until you find one that can
        # mod both x and y with no remainder
       if((temp % x == 0) and (temp % y == 0)):
           lcm = temp
           break
       temp += 1
   return lcm


def downsample(x,q):
    if(q<1 or type(q) is not int):
        return []
    returnlist = []
    for i in range(len(x)):
        if i%q==0:
            returnlist.append(x[i])
    return returnlist

# def upsample(x,p):
#     if(p<1 or type(p) is not int):
#         return []
#     else:
#         returnlist = [x[0]]
#         for i in range(len(x)-1):
#             dy = (x[i+1]-x[i])/p
#             for j in range(1,p):
#                 returnlist.append(x[i]+j*dy)
#             #returnlist.append(x[i+1])
#         # dy = (x[len(x)-1]-x[len(x)-2])/p
#         # for i in range(1,p):
#         #     returnlist.append(x[len(x)-1]+i*dy)
#         return returnlist

def upsample(x,p):
    if(p<1 or type(p) is not int):
        return []
    else:
        returnlist = [x[0]]
        for i in range(len(x)-1):
            dy = (x[i+1]-x[i])/p
            for j in range(1,p):
                returnlist.append(x[i]+j*dy)
            returnlist.append(x[i+1])
        dy = (x[len(x)-1]-x[len(x)-2])/p
        for i in range(1,p):
            returnlist.append(x[len(x)-1]+i*dy)
        return returnlist

# def resample(inp,desired_length):
#     if(len(inp)==desired_length):
#         return inp
#     lcmu = lcm(len(inp),desired_length)
#     inp = upsample(inp,int(lcmu/(len(inp)-1)))
#     inp = downsample(inp,int(lcmu/desired_length))
#     return inp               
                 
def resample(inp,desired_length):
    if(len(inp)==desired_length):
        return inp
    lcmu = lcm(len(inp),desired_length)
    inp = upsample(inp,int(lcmu/len(inp)))
    inp = downsample(inp,int(lcmu/desired_length))
    return inp   




        

