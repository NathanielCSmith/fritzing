# usage:
#    makepanelizerxml2.py -f destination-folder -u username -pwd password -w worksheet
#    creates a panelizer.xml file that can be used for further steps

import getopt, sys
from datetime import date
import gdata.docs
import gdata.docs.service
import gdata.spreadsheet.service
import re, os, os.path
import ConfigParser
import sys
import traceback
from pprint import pprint

def usage():
    print """
usage:
    makepanelizerxml2.py -f folder-destination -u username -p password -w worksheet
    creates a panelizer.xml file that can be used for further steps
"""
    
           
def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hf:u:p:w:", ["help", "folder", "username", "password","worksheet"])
    except getopt.GetoptError, err:
        # print help information and exit:
        print str(err) # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
        
    destFolder = None
    username = None
    password = None
    worksheet = None
    
    for o, a in opts:
        #print o
        #print a
        if o in ("-f", "--folder"):
            destFolder = a
        elif o in ("-u", "--username", "user"):
            username = a
        elif o in ("-p", "--password", "pwd"):
            password = a
        elif o in ("-w", "--worksheet", "sheet"):
            worksheet = a
        elif o in ("-h", "--help"):
            usage()
            sys.exit(2)
        else:
            assert False, "unhandled option"
    
    if not(destFolder) or not(username) or not(password) or not(worksheet):
        usage()
        sys.exit(2)
        
    try:
        # Connect to Google
        gd_client = gdata.spreadsheet.service.SpreadsheetsService()
        gd_client.email = username
        gd_client.password = password
        gd_client.ProgrammaticLogin()
    except:
        traceback.print_exc()    
        sys.exit(1)

    print "connected"

    # Query for the rows
    print "Reading rows...."
    q = gdata.spreadsheet.service.DocumentQuery()
    q['title'] = "Fritzing Fab Orders"
    q['title-exact'] = 'true'
    feed = gd_client.GetSpreadsheetsFeed(query=q)
    assert(feed)
    
    spreadsheet_id = feed.entry[0].id.text.rsplit('/',1)[1]    
    wsfeed = gd_client.GetWorksheetsFeed(spreadsheet_id)
    
    worksheet_id = None
    for entry in wsfeed.entry:
        if entry.title.text and (entry.title.text == worksheet):
            worksheet_id = entry.id.text.rsplit('/',1)[1]
            break
            
    if not(worksheet_id):
        print "worksheet", worksheet, "not found"
        sys.exit(0)

    lines = []
    rows = gd_client.GetListFeed(spreadsheet_id, worksheet_id).entry
    for row in rows:
        filename = row.custom['filename'].text
        orderNumber = row.custom['order-nr'].text
        if filename and orderNumber:
            optional = row.custom['optional'].text
            if not(optional):
                optional = 0
            required = row.custom['count'].text
            if not(required):
                required = 0
            xml =  "<board name='{0}_{1}_{2}' requiredCount='{1}' maxOptionalCount='{3}' inscription='{0}' inscriptionHeight='2mm' originalName='{2}' />\n"
            if orderNumber == "products":
                xml =  "<board name='{2}' requiredCount='{1}' maxOptionalCount='{3}' inscription='' inscriptionHeight='2mm' originalName='{2}' />\n"
            xml = xml.format(orderNumber, required, filename, optional)
            lines.append(xml)
            #print xml
        
    outfile = open(os.path.join(destFolder, "panelizer.xml"), "w")
    today = date.today()
    outfile.write("<panelizer width='550mm' height='330mm' small-width='205mm' small-height='330mm' spacing='6mm' border='0mm' outputFolder='{0}' prefix='{1}'>\n".format(destFolder, today.strftime("%Y.%m.%d")))
    outfile.write("<paths>\n")
    outfile.write("<path>{0}</path>\n".format(destFolder))
    productDir = destFolder
    productDir = os.path.split(productDir)[0]
    productDir = os.path.split(productDir)[0]
    productDir = os.path.join(productDir, "products")
    outfile.write("<path>{0}</path>\n".format(productDir))
    outfile.write("</paths>\n")
    outfile.write("<boards>\n")    
    for l in lines:
        outfile.write(l)
    outfile.write("</boards>\n")    
    outfile.write("</panelizer>\n")    
    outfile.close()
    

if __name__ == "__main__":
    main()


