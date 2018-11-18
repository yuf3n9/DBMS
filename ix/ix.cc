
#include "ix.h"
#include <stack>
#include <vector>

IndexManager* IndexManager::_index_manager = 0;

IndexManager* IndexManager::instance()
{
    if(!_index_manager)
        _index_manager = new IndexManager();

    return _index_manager;
}

IndexManager::IndexManager()
{
}

IndexManager::~IndexManager()
{
}

RC IndexManager::createFile(const string &fileName)
{
    ifstream ifile(fileName);
    if (ifile) {
        ifile.close();
       return FILE_EXISTED;
    } else{
       ofstream outfile (fileName);
       char* firstPage = new char[PAGE_SIZE];
       memset((void*)firstPage,0,PAGE_SIZE);
       outfile.write(firstPage, PAGE_SIZE);
       outfile.flush();
       outfile.close();
       delete[] firstPage;
       return SUCCESS;
    }
    return -1;
}

RC IndexManager::destroyFile(const string &fileName)
{
    RC rc = -1;
    if( std::remove(fileName.c_str()) == 0){
        rc = SUCCESS;
    } else{
       rc= FILE_DELETE_FAIL;
    }
    return rc;
}

RC IndexManager::openFile(const string &fileName, IXFileHandle &ixfileHandle)
{
    RC rc = -1;
    if(ixfileHandle.fileHandle.filefs.is_open()){
        rc = -1;
    } else{
       ixfileHandle.fileHandle.filefs.open(fileName);
       if(ixfileHandle.fileHandle.filefs.is_open()){
            char* firstPage = new char[PAGE_SIZE];
            memset(firstPage,0,PAGE_SIZE);
            ixfileHandle.fileHandle.filefs.read(firstPage, PAGE_SIZE);
            memcpy(&(ixfileHandle.fileHandle.readPageCounter), firstPage, sizeof(int));
            memcpy(&(ixfileHandle.fileHandle.writePageCounter), firstPage+4, sizeof(int));
            memcpy(&(ixfileHandle.fileHandle.appendPageCounter), firstPage+8, sizeof(int));
            memcpy(&(ixfileHandle.rootNodePointer), firstPage+12, sizeof(int));
            delete[] firstPage;
            rc = SUCCESS;
       } else{
            rc = OPEN_HANDLE_FAIL;
       }
   }
       //cout<<fileHandle.appendPageCounter<<endl;
    return rc;
}

RC IndexManager::closeFile(IXFileHandle &ixfileHandle)
{
    char* firstPage = new char[PAGE_SIZE];
    memset(firstPage,0,PAGE_SIZE);
    memcpy(firstPage, &(ixfileHandle.fileHandle.readPageCounter), sizeof(int));
    memcpy(firstPage+4, &(ixfileHandle.fileHandle.writePageCounter), sizeof(int));
    memcpy(firstPage+8, &(ixfileHandle.fileHandle.appendPageCounter), sizeof(int));
    memcpy(firstPage+12, &(ixfileHandle.rootNodePointer), sizeof(int));
    ixfileHandle.fileHandle.filefs.seekg(0, ixfileHandle.fileHandle.filefs.beg);
    ixfileHandle.fileHandle.filefs.write(firstPage, PAGE_SIZE);
    ixfileHandle.fileHandle.filefs.flush();
    ixfileHandle.fileHandle.filefs.close();
    delete[] firstPage;
    return SUCCESS;
}

RC IndexManager::insertEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
    void* retKey;
    int retPageId1;
    int retPageId2;
    cout<<"rootNodePointer: "<<ixfileHandle.rootNodePointer<<endl;
    RID retRid;
    return insertEntryHelper(attribute, ixfileHandle, key, rid, ixfileHandle.rootNodePointer, retPageId1, retKey, retRid, retPageId2);
}

RC IndexManager::deleteEntry(IXFileHandle &ixfileHandle, const Attribute &attribute, const void *key, const RID &rid)
{
    return -1;
}


RC IndexManager::scan(IXFileHandle &ixfileHandle,
        const Attribute &attribute,
        const void      *lowKey,
        const void      *highKey,
        bool			lowKeyInclusive,
        bool        	highKeyInclusive,
        IX_ScanIterator &ix_ScanIterator)
{
    return -1;
}

void IndexManager::printBtree(IXFileHandle &ixfileHandle, const Attribute &attribute) const {
    unsigned int rootNodePointer;
    rootNodePointer = ixfileHandle.rootNodePointer;
    cout<<"rootNodePointer: "<<rootNodePointer<<endl;
    if (rootNodePointer <= 0) {
        cout<<"Failed to get root node."<<endl;
    }
    //cout << "{" << endl;
    dfsPrint(ixfileHandle, attribute, rootNodePointer, 0, true);
    //cout << "}" << endl;
}

void IndexManager::dfsPrint(IXFileHandle &ixfileHandle, const Attribute &attribute, unsigned pageId, int depth, bool last) const {
    void* page = malloc(PAGE_SIZE);
    int type = attribute.type;
    ixfileHandle.fileHandle.readPage(pageId - 1, page); //-1????????????????
    bool isLeaf;
    memcpy(&isLeaf, page, sizeof(bool));
    int space;
    memcpy(&space, (char *)page+sizeof(bool), sizeof(int));
    int offset = sizeof(bool) + sizeof(int);
    vector<int> pageVector;
    unsigned childPageId;

    printTabs(depth);
    cout << "{\"keys\":[";

    if (!isLeaf) {
        
        switch (type)
        {
            case TypeInt:
            {
                vector<int> keyVector;
                while (offset < space - (int)sizeof(int)) { //need to - sizeof(int) to handle the last pointer separately?????????
                    memcpy(&childPageId, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    pageVector.push_back(childPageId);
                    int k;
                    memcpy(&k, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    keyVector.push_back(k);
                    offset += sizeof(RID);
                }
                memcpy(&childPageId, (char *)page+offset, sizeof(int));
                pageVector.push_back(childPageId);
                for (unsigned i=0; i<keyVector.size() - 1; i++) {
                    cout << "\"" << keyVector.at(i) << "\",";
                }
                cout << "\"" << keyVector.at(keyVector.size() - 1) << "\"]," << endl;
                break;
            }
            case TypeReal:
            {
                vector<float> keyVector;
                while (offset < space - (int)sizeof(int)) {
                    memcpy(&childPageId, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    pageVector.push_back(childPageId);
                    float k;
                    memcpy(&k, (char *)page+offset, sizeof(float));
                    offset += sizeof(float);
                    keyVector.push_back(k);
                    offset += sizeof(RID);
                }
                memcpy(&childPageId, (char *)page+offset, sizeof(int));
                pageVector.push_back(childPageId);
                for (unsigned i=0; i<keyVector.size() - 1; i++) {
                    cout << "\"" << keyVector.at(i) << "\",";
                }
                cout << "\"" << keyVector.at(keyVector.size() - 1) << "\"]," << endl;
                break;
            }
            case TypeVarChar:
            {
                vector<char*> keyVector;
                while (offset < space - (int)sizeof(int)) {
                    memcpy(&childPageId, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    pageVector.push_back(childPageId);
                    int length;
                    memcpy(&length, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    void* k = malloc(length);
                    memcpy(k, (char *)page+offset, length);
                    offset += length;
                    keyVector.push_back((char *)k);
                    offset += sizeof(RID);
                    free(k);
                }
                memcpy(&childPageId, (char *)page+offset, sizeof(int));
                pageVector.push_back(childPageId);
                for (unsigned i=0; i<keyVector.size() - 1; i++) {
                    cout << "\"" << keyVector.at(i) << "\",";
                }
                cout << "\"" << keyVector.at(keyVector.size() - 1) << "\"]," << endl;
                break;
            }

            default:
                break;
        }

        printTabs(depth);
        cout << "\"children\":[" << endl;
        
        for(unsigned i = 0; i < pageVector.size() - 1; i++)
        {
            dfsPrint(ixfileHandle, attribute, pageVector.at(i), depth + 1, false);
        }
        dfsPrint(ixfileHandle, attribute, pageVector.at(pageVector.size() - 1), depth + 1, true);
        printTabs(depth);
        cout << "]}";
        if (!last) {
            cout << ",";
        }
        cout << endl;
    }
    else {
        vector<RID> RIDVector;
        RID rid;
        switch (type)
        {
            case TypeInt:
            {
                vector<int> keyVector;
                while (offset < space - (int)sizeof(int)) {//TODO: handle the pointer at the end
                    int k;
                    memcpy(&k, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    keyVector.push_back(k);
                    memcpy(&rid, (char *)page+offset, sizeof(RID));
                    offset += sizeof(RID);
                    RIDVector.push_back(rid);
                }
                
                for (unsigned i=0; i<keyVector.size() - 1; i++) {
                    cout << "\"" << keyVector.at(i) << ":";
                    cout << "(" << RIDVector.at(i).pageNum << ", " << RIDVector.at(i).slotNum << ")\",";
                }
                cout << "\"" << keyVector.at(keyVector.size() - 1) << ":";
                cout << "(" << RIDVector.at(keyVector.size() - 1).pageNum << ", " << RIDVector.at(keyVector.size() - 1).slotNum << ")\"";
                break;
            }
            case TypeReal:
            {
                vector<float> keyVector;
                while (offset < space - (int)sizeof(int)) {//TODO: handle the pointer at the end
                    float k;
                    memcpy(&k, (char *)page+offset, sizeof(float));
                    offset += sizeof(float);
                    keyVector.push_back(k);
                    memcpy(&rid, (char *)page+offset, sizeof(RID));
                    offset += sizeof(RID);
                    RIDVector.push_back(rid);
                }
                
                for (unsigned i=0; i<keyVector.size() - 1; i++) {
                    cout << "\"" << keyVector.at(i) << ":";
                    cout << "(" << RIDVector.at(i).pageNum << ", " << RIDVector.at(i).slotNum << ")\",";
                }
                cout << "\"" << keyVector.at(keyVector.size() - 1) << ":";
                cout << "(" << RIDVector.at(keyVector.size() - 1).pageNum << ", " << RIDVector.at(keyVector.size() - 1).slotNum << ")\"";
                break;
            }
            case TypeVarChar:
            {
                vector<char*> keyVector;
                while (offset < space - (int)sizeof(int)) {//TODO: handle the pointer at the end
                    int length;
                    memcpy(&length, (char *)page+offset, sizeof(int));
                    offset += sizeof(int);
                    void* k = malloc(length);
                    memcpy(k, (char *)page+offset, length);
                    offset += length;
                    keyVector.push_back((char *)k);
                    memcpy(&rid, (char *)page+offset, sizeof(RID));
                    offset += sizeof(RID);
                    RIDVector.push_back(rid);
                }
                
                for (unsigned i=0; i<keyVector.size() - 1; i++) {
                    cout << "\"" << keyVector.at(i) << ":";
                    cout << "(" << RIDVector.at(i).pageNum << ", " << RIDVector.at(i).slotNum << ")\",";
                }
                cout << "\"" << keyVector.at(keyVector.size() - 1) << ":";
                cout << "(" << RIDVector.at(keyVector.size() - 1).pageNum << ", " << RIDVector.at(keyVector.size() - 1).slotNum << ")\"";
                break;
            }
            default:
                break;
        }
        cout << "]}";
        if (!last) {
            cout << ",";
        }
        cout << endl;
    }
    free(page);
}

void IndexManager::printTabs(int num) const {
    for(int i = 0; i < num; i++)
    {
        cout << '\t';
    }
}

RC IndexManager::insertEntryHelper(const Attribute &attribute, IXFileHandle &ixfileHandle, const void* key, const RID &rid, int curPageId, 
            int &retPageId1, void* retKey, RID &retRid, int &retPageId2){
    if(ixfileHandle.rootNodePointer == 0){
        int newPageId;
        RC rc = createNewPage(true, ixfileHandle, newPageId);
        ixfileHandle.rootNodePointer = newPageId;
        if(rc != SUCCESS) return rc;
        //rc = insertLeaf(ixfileHandle, newPageId, attribute, key, rid);
        //if(rc != SUCCESS) return rc;
        ixfileHandle.rootNodePointer = newPageId;
        insertEntryHelper(attribute, ixfileHandle, key, rid, newPageId, retPageId1, retKey, retRid, retPageId2);
    } else{
        void* page = malloc(PAGE_SIZE);
        ixfileHandle.fileHandle.readPage(curPageId-1, page);
        bool isLeaf;
        memcpy((char*)(&isLeaf), (char*)page, sizeof(bool));
        if(isLeaf){
            if(attribute.type == 0){
                int insertSize = sizeof(int)+2*sizeof(unsigned);
                int space;
                memcpy((char*)(&space), (char*)page+1, sizeof(int));
                if(space+insertSize <= PAGE_SIZE){
                    insertLeaf(ixfileHandle, curPageId, attribute, key, rid);
                } else{
                    int newPageId;
                    void* pushupKey = malloc(sizeof(int));
                    RID pushupRid;
                    splitLeafPage(ixfileHandle, curPageId, newPageId, pushupKey, pushupRid, attribute);
                    bool isSmall;
                    RC rc = keyCompare(isSmall, attribute, key, pushupKey, rid, pushupRid);
                    if(isSmall){
                        //insertEntryHelper(attribute, ixfileHandle, key, rid, curPageId, retPageId1, retKey, retRid, retPageId2);
                        insertLeaf(ixfileHandle, curPageId, attribute, key, rid);
                    } else{
                        //insertEntryHelper(attribute, ixfileHandle, key, rid, newPageId, retPageId1, retKey, retRid, retPageId2);
                        insertLeaf(ixfileHandle, newPageId, attribute, key, rid);
                    }

                    retKey = pushupKey;
                    retRid = pushupRid;
                    retPageId1 = curPageId;
                    retPageId2 = newPageId;
                }
            }
        } else{
            
        }
    }
    if(curPageId==0){
        int newRootPageId = 0;
        createNewPage(false, ixfileHandle, newRootPageId);
        ixfileHandle.rootNodePointer = newRootPageId;
        insertInternalNode(ixfileHandle, newRootPageId, attribute, retKey, retRid, retPageId1, retPageId2);
    }
    return SUCCESS;
}

RC IndexManager::createNewPage(bool isLeaf, IXFileHandle &ixfileHandle, int &newPageId){
    char* newPage = (char*)malloc(PAGE_SIZE);
    memset(newPage, 0, PAGE_SIZE);
    memcpy(newPage, &isLeaf, sizeof(bool));
    int size;
    if(isLeaf){
        size = 2*sizeof(int)+sizeof(bool);
    } else{
        size = sizeof(int)+sizeof(bool);
    }
    memcpy(newPage+1, &size, sizeof(int));
    int nextPtr = 0;
    memcpy(newPage+5, &nextPtr, sizeof(int));
    ixfileHandle.fileHandle.appendPage(newPage);
    newPageId = ixfileHandle.fileHandle.appendPageCounter;
    free(newPage);
    return SUCCESS;
}
        
RC IndexManager::keyCompare(bool &res, const Attribute &attribute, const void* key1, const void* key2, const RID &rid1, const RID &rid2){
    if(attribute.type == 0){
        int k1;
        int k2;
        memcpy((char*)(&k1), (char*)key1, sizeof(int));
        memcpy((char*)(&k2), (char*)key2, sizeof(int));
        if(k1 != k2){
            res = k1<k2;
        } else{
            if(rid1.pageNum != rid2.pageNum){
                res = rid1.pageNum < rid2.pageNum;
            } else{
                res = rid1.slotNum < rid2.slotNum;
            }
        }
    } else{
        res = true;
        return -1;
    }
    return SUCCESS;
}

RC IndexManager::insertInternalNode(IXFileHandle& ixfileHandle, int pageId, const Attribute &attribute, const void* key, 
    const RID &rid, int page1, int page2){
    if(attribute.type == 0){
        void* page = malloc(PAGE_SIZE);
        ixfileHandle.fileHandle.readPage(pageId-1, page);
        void* newPage = malloc(PAGE_SIZE);
        int space;
        memcpy((char*)(&space), (char*)page+1, sizeof(int));
        //first item to insert in this page, insert one key and two page ids
        if(space == 5){
            int offset = 5;
            memcpy((char*)page+offset, (char*)(&page1), sizeof(int));
            offset += sizeof(int);
            memcpy((char*)page+offset, (char*)(key), sizeof(int));
            offset += sizeof(int);
            memcpy((char*)page+offset, (char*)(&(rid.pageNum)), sizeof(unsigned));
            offset += sizeof(unsigned);
            memcpy((char*)page+offset, (char*)(&(rid.slotNum)), sizeof(unsigned));
            offset += sizeof(unsigned);
            memcpy((char*)page+offset, (char*)(&page2), sizeof(int));
            offset += sizeof(int);
            memcpy((char*)page+1, (char*)(&offset), sizeof(int));
        } else{
            //if it is not first item to insert, insert one key and one page id
            int offset = 5;
            int newOffset = offset;
            bool hasInserted = false;
            bool isFirst = true;
            while(offset<space){
                int beginPageId;
                int pageId;
                if(offset == 5){
                    memcpy((char*)(&beginPageId), (char*)page+offset, sizeof(int));
                    offset += sizeof(int);
                }
                if(!hasInserted){
                    int k, pageNum, slotNum;
                    memcpy((char*)(&k), (char*)page+offset, sizeof(int));
                    offset += sizeof(int);
                    memcpy((char*)(&pageNum), (char*)page+offset, sizeof(int));
                    offset += sizeof(int);
                    memcpy((char*)(&slotNum), (char*)page+offset, sizeof(int));
                    offset += sizeof(int);
                    bool isSmall;
                    RID r;
                    r.pageNum = pageNum;
                    r.slotNum = slotNum;
                    RC rc = keyCompare(isSmall, attribute, key, (void*)(&k), rid, r);
                    if(rc!=SUCCESS) return -1;
                    if(isSmall){
                        if(isFirst){
                            //put page1, new key, then everything from old page
                            memcpy((char*)newPage+newOffset, (char*)(&page1), sizeof(int));
                            newOffset += sizeof(int);
                            memcpy((char*)newPage+newOffset, (char*)(key), sizeof(int));
                            newOffset += sizeof(int);
                            memcpy((char*)newPage+newOffset, (char*)(&rid.pageNum), sizeof(unsigned));
                            newOffset += sizeof(unsigned);
                            memcpy((char*)newPage+newOffset, (char*)(&rid.slotNum), sizeof(unsigned));
                            newOffset += sizeof(unsigned);
                            memcpy((char*)newPage+newOffset, (char*)page+5, space-5);
                            //set space for new page
                            int newSpace = space+2*sizeof(int)+2*sizeof(unsigned);
                            memcpy((char*)newPage+1, &newSpace, sizeof(int));
                            offset = space;
                            hasInserted = true;
                        } else{
                            //put new key, then page2, then everything from old page
                            memcpy((char*)newPage+newOffset, (char*)(key), sizeof(int));
                            newOffset += sizeof(int);
                            memcpy((char*)newPage+newOffset, (char*)(&page2), sizeof(int));
                            newOffset += sizeof(int);
                            memcpy((char*)newPage+newOffset, (char*)(&rid.pageNum), sizeof(unsigned));
                            newOffset += sizeof(unsigned);
                            memcpy((char*)newPage+newOffset, (char*)(&rid.slotNum), sizeof(unsigned));
                            newOffset += sizeof(unsigned);
                            int start = offset-sizeof(int)-2*sizeof(unsigned);
                            memcpy((char*)newPage+newOffset, (char*)page+start, space-start);
                            //set space for new page
                            int newSpace = space+2*sizeof(int)+2*sizeof(unsigned);
                            memcpy((char*)newPage+1, &newSpace, sizeof(int));
                            offset = space;
                            hasInserted = true;
                        }
                    } else{
                        int size = 2*sizeof(int)+2*sizeof(unsigned);
                        if(isFirst){
                            size += sizeof(int);
                        }
                        memcpy((char*)newPage+newOffset, (char*)page+offset, size);
                        offset += size;
                        newOffset += size;
                    }
                } else{
                    ;
                }
                isFirst = false;
            }
        }
        free(page);
        free(newPage);
    } else{
        return -1;
    }
    return SUCCESS;
}

RC IndexManager::insertLeaf(IXFileHandle &ixfileHandle, int pageId, const Attribute &attribute, const void* key, const RID &rid){
    if(attribute.type == 0){
        //cout<<"pageid: "<<pageId<<endl;
        void* page = malloc(PAGE_SIZE);
        ixfileHandle.fileHandle.readPage(pageId-1, page);
        void* newPage = malloc(PAGE_SIZE);
        int space;
        memcpy(&space, (char*)page+1, sizeof(int));
        //cout<<space<<endl;
        int offset = 9;
        int newOffset = offset;
        memcpy((char*)(newPage), (char*)page, 2*sizeof(int)+sizeof(bool));
        bool hasInserted = false;
        while(offset < space){
            int k;
            memcpy(&k, (char*)page+offset, sizeof(int));
            RID r;
            memcpy(&(r.pageNum), (char*)page+offset+sizeof(int), sizeof(unsigned));
            memcpy(&(r.slotNum), (char*)page+offset+sizeof(int)+sizeof(unsigned), sizeof(unsigned));
            bool isSmall;
            RC rc = keyCompare(isSmall, attribute, key, (void*)(&k), rid, r);
            if(!rc==SUCCESS)   return -1;
            if(!isSmall && !hasInserted){
                memcpy((char*)newPage+newOffset, (char*)&k, sizeof(int));
                newOffset += sizeof(int);
                memcpy((char*)newPage+newOffset, (char*)(&rid.pageNum), sizeof(unsigned));
                newOffset += sizeof(unsigned);
                memcpy((char*)newPage+newOffset, (char*)(&rid.slotNum), sizeof(unsigned));
                newOffset += sizeof(unsigned);
                hasInserted = true;
            } else{
                int itemSize = sizeof(int)+2*sizeof(unsigned);
                memcpy((char*)newPage+newOffset, (char*)page+offset, itemSize);
                offset += itemSize;
                newOffset += itemSize;
            }
        }
        if(!hasInserted){
            memcpy((char*)newPage+newOffset, (char*)key, sizeof(int));
            newOffset += sizeof(int);
            memcpy((char*)newPage+newOffset, (char*)(&rid.pageNum), sizeof(unsigned));
            newOffset += sizeof(unsigned);
            memcpy((char*)newPage+newOffset, (char*)(&rid.slotNum), sizeof(unsigned));
            newOffset += sizeof(unsigned);
            hasInserted = true;
        }
        int itemSize = sizeof(int)+2*sizeof(unsigned);
        space += itemSize;
        memcpy((char*)newPage+1, (char*)(&space), sizeof(int));
        int nextpage;
        memcpy((char*)(&nextpage), newPage+5, sizeof(int));
        cout<<"space: "<<space<<", pageid: "<<pageId<<"nextpage: "<<nextpage<<endl;
        ixfileHandle.fileHandle.writePage(pageId-1, newPage);
        free(page);
        free(newPage);
    } else{
        return -1;
    }
    return SUCCESS;
}

//input page to split, return newpageid, push-up key, push-up rid
RC IndexManager::splitLeafPage(IXFileHandle& ixfileHandle, int pageId, int &newPageId, void* key, RID& rid, const Attribute &attribute){
    void* page = malloc(PAGE_SIZE);
    void* splitedPage = malloc(PAGE_SIZE);
    void* newPage = malloc(PAGE_SIZE);
    memset(page, 0, PAGE_SIZE);
    memset(newPage, 0, PAGE_SIZE);
    memset(splitedPage, 0, PAGE_SIZE);
    ixfileHandle.fileHandle.readPage(pageId-1, page);
    if(attribute.type == 0){
        int offset = 9;
        int splitOffset = 9;
        int newOffset = 9;
        memcpy((char*)newPage, (char*)page, sizeof(bool)+sizeof(int));
        memcpy((char*)splitedPage, (char*)page, sizeof(bool)+sizeof(int));
        int itemSize = sizeof(int)+2*sizeof(unsigned);
        while(offset <= PAGE_SIZE/2){
            memcpy((char*)splitedPage+splitOffset, (char*)page+offset, itemSize);
            offset += itemSize;
            splitOffset += itemSize;
        }
        memcpy((char*)key, (char*)page+offset, sizeof(int));
        memcpy((char*)(&rid.pageNum), (char*)page+offset+sizeof(int), sizeof(unsigned));
        memcpy((char*)(&rid.slotNum), (char*)page+offset+sizeof(int)+sizeof(unsigned), sizeof(unsigned));
        while(offset+itemSize <= PAGE_SIZE){
            memcpy((char*)newPage+newOffset, (char*)page+offset, itemSize);
            offset += itemSize;
            newOffset += itemSize;
        }
        memcpy((char*)newPage+1, (char*)(&newOffset), sizeof(int));
        memcpy((char*)splitedPage+1, (char*)(&splitOffset), sizeof(int));
        int originNextPtr = 0;
        memcpy((char*)(&originNextPtr), (char*)newPage+5, sizeof(int));
        memcpy((char*)splitedPage+5, &originNextPtr, sizeof(int));
        int nextPagePtr = ixfileHandle.fileHandle.appendPageCounter+1;
        memcpy((char*)newPage+5, &nextPagePtr, sizeof(int));
        ixfileHandle.fileHandle.writePage(pageId-1, splitedPage);
        ixfileHandle.fileHandle.appendPage(newPage);
        newPageId = ixfileHandle.fileHandle.appendPageCounter;
    } else{
        free(page);
        free(splitedPage);
        free(newPage);
        return -1;
    }
    free(page);
    free(splitedPage);
    free(newPage);
    return SUCCESS;
}

IX_ScanIterator::IX_ScanIterator()
{
}

IX_ScanIterator::~IX_ScanIterator()
{
}

RC IX_ScanIterator::getNextEntry(RID &rid, void *key)
{
    return -1;
}

RC IX_ScanIterator::close()
{
    return -1;
}


IXFileHandle::IXFileHandle()
{
    ixReadPageCounter = 0;
    ixWritePageCounter = 0;
    ixAppendPageCounter = 0;
}

IXFileHandle::~IXFileHandle()
{
}

RC IXFileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    return this->fileHandle.collectCounterValues(readPageCount, writePageCount, appendPageCount);
}

RC IXFileHandle::getRootNodePointer(unsigned &rootNodePointer){
    rootNodePointer = this->rootNodePointer;
    return SUCCESS;
}
