#README.md
import records
import datetime


db = records.Database('mysql://student:6s08student@localhost/iesc')

KERBEROS = "premila"

# dictionary of location boundaries defined as (lat, lon) pairs for boundaries
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
    outpt = ""
    my_string=''
    count=0
    my_area=''
 
    if method == "GET":
        
        ans=False
        rows=db.query("SELECT * FROM geo").as_dict()
        for row in reversed(rows):
            if row['kerberos']==KERBEROS:
                my_area=row['area']
                ans=True
                break
        if ans==False:
            outpt= "area: No Area Reported"+ "##-"+ " "+ "users nearby"
            return outpt
                
        for item in reversed(rows):        
            time=item['time']        
            difference=datetime.datetime.now()-time
            difference=difference.total_seconds()
            difference=round((difference/60))
            difference=difference-300
            if item['area']==my_area:
                if item['kerberos'] not in my_string:
                    if difference<=600:
                        my_string+=item['kerberos']+"##"
                        count=count+1
            
        
        outpt="area:"+ str(my_area)+ "##"+ my_string +str(count)+ " "+ "users nearby"
      
        return outpt


    elif method == "POST":
        parameters = request['POST']
        outpt = ""

        # check parameters
        if "kerb" in parameters and "MAC" in parameters and "lat" in parameters and "lon" in parameters:
            kerberos = parameters["kerb"]
            MAC = parameters["MAC"]
            lat = float(parameters["lat"])
            lon = float(parameters["lon"])
            area=get_area((lon,lat),locations)

            outpt +="Inserting into Database..." 

            # START EDITING HERE
            query="INSERT INTO geo (kerberos, MAC, lon, lat, area) VALUES ('" + kerberos + "','" +MAC +"' , '" + str(lon) + "','" + str(lat) + "','" + area + "')"
            db.query(query)


            # LEAVE THIS PART ALONE
            outpt += "Data Received!"

        else:
            outpt += "Missing POST parameters"


    else:
        outpt += "HTTP Request Error"

    return outpt
