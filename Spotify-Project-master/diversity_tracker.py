import records
import requests
from datetime import datetime

PLAYLIST_DATABASE = "playlists"
LOCATIONS_DATABASE = "user_tracking"

db = records.Database('mysql://student:6s08student@localhost/iesc')

SP_PLAYLIST = {"6EN4MB1Al0YaEsrb6u7Afu":["7F7S2mtZnzW7BucnxzTbo3","4Ce37cRWvM1vIGGynKcs22","46Kcradxrva9Dny4lHU1b3"],"7KqgJjPlhv8CQpbRudXNQq":["3AaiEsiqHO2ylnnOdWninE","4rwpZEcnalkuhPyGkEdhu0"]}
SP_PLAYLIST_NAMES = {"lol":"6EN4MB1Al0YaEsrb6u7Afu","fnfnfnf":"7KqgJjPlhv8CQpbRudXNQq"}

def web_handler(request):
    method = request['REQUEST_METHOD']

    if method == 'GET':
        output = ""
        parameters = request['GET']
        #something having to do with getting nearby people
        # if "ID" in parameters and "spoof" in parameters:
        #     area = ""
        #     ID = parameters["ID"]
        #     people = db.query('SELECT * FROM %s' % LOCATIONS_DATABASE)
        #     for person in people.as_dict():
        #         if(person["ID"]==ID):
        #             area = person["area"]
        #getting the user's home data and diversity
        if "selfuserID" in parameters and "spof" in parameters:
            selfuserID = parameters["selfuserID"]
            user = db.query('SELECT * FROM %s ORDER BY TIME DESC' % PLAYLIST_DATABASE)
            userdict=user.as_dict()
            songlist = []
            for item in userdict:
                if item['username']==selfuserID:
                    r= requests.get("https://api.spotify.com/v1/tracks/"+item["track_id"])
                    songlist.append(r.json())
            diversity = diversity_track(selfuserID,songlist)
            output += "Diversity##"+str(diversity)
        #getting a string of playlist id's given a user id
        elif "userID" in parameters and "spof" in parameters:
            count = 0
            userID = parameters["userID"]
            if parameters["spof"] is "True":
                for item in SP_PLAYLIST_NAMES:
                    output+=item+":"
                    output+=SP_PLAYLIST_NAMES[item]
                    count+=1
                    if(count!=len(SP_PLAYLIST_NAMES)):
                        output+=","
                #output = "display[{'username': '1241183313', 'playlist_id': '4ErzFbZk1sJbjjQGyUK3Z2', 'category': 'Pop', 'track_id': '46Kcradxrva9Dny4lHU1b3', 'kerberos': None, 'ID': 1, 'time': datetime.datetime(2017, 4, 26, 19, 46, 48)}, {'username': '1241183313', 'playlist_id': '4ErzFbZk1sJbjjQGyUK3Z2', 'category': 'Pop', 'track_id': '62lc21zAcKWs11pbSOabO9', 'kerberos': None, 'ID': 2, 'time': datetime.datetime(2017, 4, 26, 20, 9, 4)}]"
                output+="&&"+str(count)
            else:
            #NEEDS TO BE UPDATED FOR AUTHORIZATION TOKEN
                #r = requests.get("https://api.spotify.com")
                output="sad"

            return output
        #getting a string of songs given a playlist
        elif "playlistID" in parameters and "spof" in parameters:
            count = 0
            playlistID = parameters["playlistID"]
            if parameters["spof"] is "True":
                for item in SP_PLAYLIST[playlistID]:
                    count+=1
                    r = requests.get("https://api.spotify.com/v1/tracks/"+item)
                    output+=r.json()["name"] + ":"
                    output+=item
                    if(count!=len(SP_PLAYLIST[playlistID])):
                        output+=","
            else:
            #NEEDS TO B UPDATED FOR AUTHORIZATION TOKEN
                r = requests.get("playlist info url")
                for item in r.json():
                    output+=r.json()["name"] + ":"
                    output+=r.json()["ID"]
                    if(count!=len(r.json())):
                        output+=","
                    count+=1
            output+="&&"+str(count)
        #getting info about a song given a song id
        elif "songID" in parameters and "spof" in parameters:
            songID = parameters["songID"]
            if parameters["spof"] is "True":
                r = requests.get("https://api.spotify.com/v1/tracks/"+songID)
                output+="Song Name: "+r.json()["name"]+"##Artist(s): "
                for i in range(len(r.json()["album"]["artists"])):
                    output+=r.json()["album"]["artists"][i]["name"]
                    if(i!=len(r.json()["album"]["artists"])-1):
                        output+=','
                output+="##Album: "+(r.json()["album"]["name"])
        else:
            entries = db.query('SELECT * FROM %s' % PLAYLIST_DATABASE)
            output = "display"+str(entries.as_dict())
        return output

    elif method == 'POST':
        parameters = request['POST']
        output = ""
        #posting songs to add tracks to playlist and adding this track to diversity/groupcontribution playlist
        if("playlistID" in parameters and "userID" in parameters and "songID" in parameters and "spoof" in parameters):
            playlistID = parameters["playlistID"]
            userID = parameters["userID"]
            songID = parameters["songID"]
            currenttime = datetime.now().isoformat()
            spoof = parameters["spoof"] #spoof should be true or false
            category = "Pop"
            if(spoof=="False"):
                r = requests.post('https://api.spotify.com/v1/users/'+userID+'playlists/'+playlistID+'/tracks?uris='+songID)
            query = "INSERT INTO " + PLAYLIST_DATABASE + " (time,playlist_id,username,track_id,category) VALUES ('"+currenttime + "' , '" +playlistID+ "' , '"+ userID+"' , '" + songID + "' , '" +category+"')"
            db.query(query)
            output += "Added to user playlist and database"
        # elif "ID" in parameters and "username" in parameters and "lat" in parameters and "lon" in parameters:
        #     ID = parameters["ID"]
        #     username = parameters["username"]
        #     lat = float(parameters["lat"])
        #     lon = float(parameters["lon"])
        #     area = get_area((lon,lat),locations)

        #     outpt +="Inserting into Database..."

        #     query = "INSERT INTO "+DATABASE_TABLE + " (kerberos,MAC,lat,lon,area) VALUES ('" +kerberos + "' , '" + MAC + "' , '" + parameters["lat"] + "' , '" + parameters["lon"]+"' , '" + area +"')"
        #     db.query(query)

        #     output += "Data Received!"

        else:
            output += "Incorrect POST parameters"

        return output

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

