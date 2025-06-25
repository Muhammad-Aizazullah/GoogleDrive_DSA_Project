#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <limits> // For numeric_limits
#include <vector> // For dynamic array for shared files

using namespace std;

// ANSI color codes for console output
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define BOLD    "\033[1m"

// Helper function to pause and clear screen
void pauseAndClear() {
    cout << "\nPress Enter to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer before ignoring
    system("cls"); // For Windows, use "clear" for Linux/macOS
}

// Structure to store different versions of a file in a doubly linked list
struct FileVersion
{
    string content;     // Content of this version
    FileVersion* prev;  // Pointer to previous version
    FileVersion* next;  // Pointer to next version

    // Destructor to deallocate memory for subsequent versions
    ~FileVersion() {
        delete next; // Recursively delete next version
    }
};

// Structure to store file information
struct FileNode
{
    string name;           // File name
    string type;           // File type/extension
    string owner;          // File owner/creator
    FileVersion* versionHead; // Pointer to version history (linked list)
    FileNode* next;        // Pointer to next file in directory
    int priority;          // Priority of the file for heap management

    // Destructor to deallocate memory for versions
    ~FileNode() {
        delete versionHead; // Delete the head of the version list, which will recursively delete others
        // Note: next is handled by the containing linked list (FolderNode::files)
    }

    // Helper to check if a user has specific permission on this file
    bool canAccess(const string& userRole, const string& requiredPermission) const {
        // Owner always has full access
        if (userRole == owner) { // Assuming owner's username is passed as userRole for simplicity
            return true;
        }

        // Basic role-based access control
        if (userRole == "admin") {
            return true; // Admin can do anything
        } else if (userRole == "editor") {
            return (requiredPermission == "read" || requiredPermission == "write");
        } else if (userRole == "viewer") {
            return (requiredPermission == "read");
        }
        return false; // Default: no access
    }
};

// Structure to store folder information in a tree structure
struct FolderNode
{
    string name;        // Folder name
    FolderNode* parent;  // Pointer to parent folder
    FolderNode* child;   // Pointer to first child folder
    FolderNode* sibling; // Pointer to next sibling folder
    FileNode* files;     // Pointer to files in this folder

    // Destructor to deallocate memory for children, siblings, and files
    ~FolderNode() {
        delete child;    // Recursively delete child folders
        delete sibling;  // Recursively delete sibling folders
        // Delete all files in this folder
        FileNode* currentFile = files;
        while (currentFile) {
            FileNode* nextFile = currentFile->next;
            delete currentFile; // Calls FileNode's destructor
            currentFile = nextFile;
        }
    }
};

// Structure for file metadata used in hash table
struct fileData
{
    string name;
    string type;
    string owner;
    string date;
    int size;
    fileData* next;  // For hash table chaining

    // Destructor (optional, but good practice for linked structures)
    ~fileData() {
        delete next;
    }
};

// Structure for deleted files using stack implementation
struct DeletedFile
{
    string name;
    string content;
    time_t deletionTime; // Timestamp for auto-deletion
    DeletedFile* next;  // Pointer to next deleted file in stack

    // Destructor
    ~DeletedFile() {
        delete next;
    }
};

// Structure for recent files using queue implementation
struct RecentFile
{
    string name;
    RecentFile* next;  // Pointer to next recent file in queue

    // Destructor
    ~RecentFile() {
        delete next;
    }
};

// Structure for user authentication using linked list
struct UserNode {
    string username;
    string password;
    string role; // Admin, Editor, Viewer
    string securityAnswer; // For password recovery
    string lastLogout;     // Timestamp of last logout
    UserNode* next;       // Pointer to next user

    // Destructor
    ~UserNode() {
        delete next;
    }
};

// Structure for graph-based user connections (simplified adjacency list using vector)
struct UserGraphNode {
    string username;
    // Stores tuples of (receiver_username, filename, permission)
    vector<tuple<string, string, string>> sharedFiles;
    UserGraphNode* next;

    // Destructor
    ~UserGraphNode() {
        delete next;
    }
};

// Heap structure for managing file priorities (Max-Heap)
class FilePriorityHeap {
private:
    FileNode** heapArray;
    int capacity;
    int size;

    void heapifyUp(int index) {
        while (index > 0 && heapArray[index]->priority > heapArray[(index - 1) / 2]->priority) {
            swap(heapArray[index], heapArray[(index - 1) / 2]);
            index = (index - 1) / 2;
        }
    }

    void heapifyDown(int index) {
        int leftChild, rightChild, largestChild;
        while (true) {
            leftChild = 2 * index + 1;
            rightChild = 2 * index + 2;
            largestChild = index;

            if (leftChild < size && heapArray[leftChild]->priority > heapArray[largestChild]->priority) {
                largestChild = leftChild;
            }

            if (rightChild < size && heapArray[rightChild]->priority > heapArray[largestChild]->priority) {
                largestChild = rightChild;
            }

            if (largestChild == index) {
                break;
            }

            swap(heapArray[index], heapArray[largestChild]);
            index = largestChild;
        }
    }

public:
    FilePriorityHeap(int cap) : capacity(cap), size(0) {
        heapArray = new FileNode * [capacity];
    }

    ~FilePriorityHeap() {
        // Heap itself holds pointers, actual FileNode objects are owned by FolderNode
        delete[] heapArray;
    }

    void insert(FileNode* file) {
        if (size == capacity) {
            cout << RED << "Heap is full. Cannot insert more files." << RESET << endl;
            return;
        }
        heapArray[size] = file;
        heapifyUp(size);
        size++;
        cout << GREEN << "File added to priority heap." << RESET << endl;
    }

    FileNode* extractMax() {
        if (size == 0) {
            cout << RED << "Heap is empty." << RESET << endl;
            return nullptr;
        }
        FileNode* max = heapArray[0];
        heapArray[0] = heapArray[size - 1];
        size--;
        heapifyDown(0);
        return max;
    }

    void display() {
        if (size == 0) {
            cout << RED << "Heap is empty." << RESET << endl;
            return;
        }
        cout << CYAN << "Files in Heap by Priority (Max Priority First):" << RESET << endl;
        // Displaying elements from the heapArray for inspection
        for (int i = 0; i < size; i++) {
            cout << YELLOW << heapArray[i]->name << " (Priority: " << heapArray[i]->priority << ")" << RESET << endl;
        }
    }
};

// Hash Table class for storing file metadata
class HashTable {
public:
    fileData* table[100];  // Array of pointers to file metadata

    // Constructor to initialize hash table
    HashTable()
    {
        for (int i = 0; i < 100; i++)
        {
            table[i] = nullptr;
        }
    }

    // Destructor to clean up chained lists
    ~HashTable() {
        for (int i = 0; i < 100; i++) {
            delete table[i]; // Calls fileData's destructor recursively
        }
    }

    // Simple hash function that sums ASCII values of the key
    int hashFunction(string key)
    {
        size_t h = 0;
        for (char c : key)
        {
            h += c;
        }
        return static_cast<int>(h % 100);
    }

    // Insert new file metadata into hash table
    void insert(string key, string type, int size, string owner, string date)
    {
        int index = hashFunction(key);
        // Check for duplication before inserting
        fileData* curr = table[index];
        while(curr) {
            if (curr->name == key) {
                cout << YELLOW << "Metadata for '" << key << "' already exists. Updating it." << RESET << endl;
                curr->type = type;
                curr->owner = owner;
                curr->date = date;
                curr->size = size;
                return;
            }
            curr = curr->next;
        }

        fileData* node = new fileData{ key, type, owner, date, size, nullptr };

        if (!table[index])
        {
            table[index] = node;
        }
        else
        {
            fileData* temp = table[index];
            while (temp->next)
            {
                temp = temp->next;
            }
            temp->next = node;
        }
        cout << GREEN << "Metadata for '" << key << "' inserted." << RESET << endl;
    }

    // Search for file metadata by name
    fileData* search(string key)
    {
        int index = hashFunction(key);
        fileData* curr = table[index];
        while (curr)
        {
            if (curr->name == key)
            {
                return curr;
            }
            curr = curr->next;
        }
        return nullptr;
    }

    // Remove file metadata from hash table
    void remove(string key) {
        int index = hashFunction(key);
        fileData* curr = table[index];
        fileData* prev = nullptr;
        while (curr) {
            if (curr->name == key) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    table[index] = curr->next;
                }
                curr->next = nullptr; // Detach from the list before deleting
                delete curr;
                cout << GREEN << "Metadata for '" << key << "' removed." << RESET << endl;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
        cout << RED << "Metadata for '" << key << "' not found." << RESET << endl;
    }
};

// Recycle Bin class using stack implementation
class RecycleBin
{
public:
    DeletedFile* top;  // Pointer to top of stack
    const int AUTO_DELETE_TIME_SECONDS = 60 * 60 * 24 * 7; // 7 days in seconds for auto-deletion example

    // Constructor to initialize recycle bin
    RecycleBin()
    {
        top = nullptr;
    }

    // Destructor to clean up deleted files
    ~RecycleBin() {
        delete top; // Recursively deletes all DeletedFile nodes
    }

    // Push a deleted file onto the stack
    void push(string name, string content)
    {
        top = new DeletedFile{ name, content, time(0), top };
        cout << GREEN << "File '" << name << "' moved to Recycle Bin." << RESET << endl;
        cleanUpOldFiles(); // Call cleanup after each push or periodically
    }

    // View the most recently deleted file
    void viewTop() {
        if (!top)
        {
            cout << RED << "Recycle Bin is empty" << RESET << endl;
        }
        else
        {
            cout << GREEN << "Last Deleted File: " << top->name << " (Content: " << top->content << ")" << RESET << endl;
        }
    }

    // Pop (restore) the most recently deleted file
    DeletedFile* pop() {
        if (!top) {
            cout << RED << "Recycle Bin is empty. Nothing to restore." << RESET << endl;
            return nullptr;
        }
        DeletedFile* restoredFile = top;
        top = top->next;
        restoredFile->next = nullptr; // Detach from stack
        cout << GREEN << "File '" << restoredFile->name << "' restored from Recycle Bin." << RESET << endl;
        return restoredFile;
    }

    // Clean up files older than AUTO_DELETE_TIME_SECONDS
    void cleanUpOldFiles() {
        time_t currentTime = time(0);
        DeletedFile* current = top;
        DeletedFile* prev = nullptr;

        while (current) {
            if (difftime(currentTime, current->deletionTime) > AUTO_DELETE_TIME_SECONDS) {
                // This file is old, delete it
                if (prev) {
                    prev->next = current->next;
                    current->next = nullptr; // Detach
                    delete current;
                    current = prev->next;
                } else { // If current is the top
                    top = current->next;
                    current->next = nullptr; // Detach
                    delete current;
                    current = top;
                }
                cout << YELLOW << "Old file automatically deleted from Recycle Bin." << RESET << endl;
            } else {
                prev = current;
                current = current->next;
            }
        }
    }

    void displayAll() {
        if (!top) {
            cout << RED << "Recycle Bin is empty." << RESET << endl;
            return;
        }
        cout << CYAN << "Files in Recycle Bin (Most Recent First):" << RESET << endl;
        DeletedFile* temp = top;
        while (temp) {
            char dt[26];
            ctime_s(dt, sizeof(dt), &temp->deletionTime);
            string timeStr(dt);
            cout << YELLOW << "Name: " << temp->name << ", Deletion Time: " << timeStr.substr(0, timeStr.length() - 1) << RESET << endl;
            temp = temp->next;
        }
    }
};

// Recent Files queue using FIFO implementation (modified for LRU)
class FileQueue
{
public:
    RecentFile* front;  // Front of queue
    RecentFile* rear;   // Rear of queue
    int size, capacity; // Current size and max capacity

    // Constructor to initialize recent files queue
    FileQueue(int cap = 5)
    {
        front = rear = nullptr;
        size = 0;
        capacity = cap;
    }

    // Destructor to clean up recent files
    ~FileQueue() {
        delete front; // Recursively deletes all RecentFile nodes
    }

    // Add file to end of queue (LRU logic: move to rear if already exists)
    void enqueue(string name)
    {
        // Check if file already exists in queue (for LRU logic)
        RecentFile* current = front;
        RecentFile* prev = nullptr;
        while (current) {
            if (current->name == name) {
                // File found, move it to the end (most recently used)
                if (prev) {
                    prev->next = current->next;
                    if (current == rear) { // If it was the rear, prev becomes new rear
                        rear = prev;
                    }
                } else { // If it was the front
                    front = current->next;
                    if (!front) { // If it was the only element
                        rear = nullptr;
                    }
                }
                // Now, add it to the end
                if (!rear) {
                    front = rear = current;
                } else {
                    rear->next = current;
                    rear = current;
                }
                current->next = nullptr; // Ensure detached
                cout << YELLOW << "File '" << name << "' moved to end of Recent Files (LRU)." << RESET << endl;
                return;
            }
            prev = current;
            current = current->next;
        }

        // File not found, add new
        if (size == capacity)
        {
            dequeue(); // Remove the least recently used
        }
        RecentFile* file = new RecentFile{ name, nullptr };
        if (!rear)
        {
            front = rear = file;
        }
        else
        {
            rear->next = file;
            rear = file;
        }
        size++;
        cout << GREEN << "File '" << name << "' added to Recent Files." << RESET << endl;
    }

    // Remove file from front of queue (least recently used)
    void dequeue()
    {
        if (!front)
        {
            return;
        }
        RecentFile* temp = front;
        front = front->next;
        temp->next = nullptr; // Detach
        delete temp;
        size--;
        if (!front)
        {
            rear = nullptr;
        }
    }

    // Display all recent files
    void display()
    {
        if (!front)
        {
            cout << RED << "No recent files." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Recent Files (Least Recent to Most Recent):" << RESET << endl;
            RecentFile* temp = front;
            while (temp)
            {
                cout << YELLOW << temp->name << RESET << endl;
                temp = temp->next;
            }
        }
    }
};

// User Authentication system using linked list
class UserAuth
{
public:
    UserNode* head;  // Pointer to first user

    // Constructor to initialize user authentication system
    UserAuth()
    {
        head = nullptr;
    }

    // Destructor to clean up user nodes
    ~UserAuth() {
        delete head; // Recursively deletes all UserNode nodes
    }

    // Register a new user
    void signup(string username, string password, string role, string secAns)
    {
        UserNode* curr = head;
        while (curr)
        {
            if (curr->username == username)
            {
                cout << RED << "Username already exists. Please choose a different username." << RESET << endl;
                return;
            }
            curr = curr->next;
        }
        // Validate role input
        if (!(role == "admin" || role == "editor" || role == "viewer")) {
            cout << RED << "Invalid role specified. Please use 'admin', 'editor', or 'viewer'." << RESET << endl;
            return;
        }
        head = new UserNode{ username, password, role, secAns, "", head };
        cout << GREEN << "Signup successful! Welcome, " << username << "!" << RESET << endl;
    }

    // Authenticate user login
    bool login(string username, string password)
    {
        UserNode* curr = head;
        while (curr)
        {
            if (curr->username == username && curr->password == password)
            {
                cout << GREEN << "Login successful!" << RESET << endl;
                return true;
            }
            curr = curr->next;
        }
        cout << RED << "Invalid username or password." << RESET << endl;
        return false;
    }

    // Password recovery using security question
    bool forgot(string username, string ans)
    {
        UserNode* curr = head;
        while (curr)
        {
            if (curr->username == username && curr->securityAnswer == ans)
            {
                cout << GREEN << "Your password is: " << curr->password << RESET << endl;
                return true;
            }
            curr = curr->next;
        }
        cout << RED << "Invalid username or security answer." << RESET << endl;
        return false;
    }

    // Record logout time for user
    void logout(string username)
    {
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now); // ctime_s for secure version
        string timeStr(dt);
        UserNode* curr = head;
        while (curr)
        {
            if (curr->username == username)
            {
                curr->lastLogout = timeStr;
                break;
            }
            curr = curr->next;
        }
    }

    // Get user role
    string getUserRole(string username) {
        UserNode* curr = head;
        while (curr) {
            if (curr->username == username) {
                return curr->role;
            }
            curr = curr->next;
        }
        return ""; // User not found
    }
};

// Graph-based user connections for file sharing
class UserGraph
{
public:
    UserGraphNode* head;

    UserGraph()
    {
        head = nullptr;
    }

    // Destructor to clean up user graph nodes
    ~UserGraph() {
        delete head; // Recursively deletes all UserGraphNode nodes
    }

    // Add a new user to the graph
    void addUser(string username)
    {
        UserGraphNode* newUser = new UserGraphNode{ username, {}, nullptr };
        if (!head)
        {
            head = newUser;
        }
        else
        {
            UserGraphNode* curr = head;
            while (curr->next)
            {
                curr = curr->next;
            }
            curr->next = newUser;
        }
        cout << GREEN << "User '" << username << "' added to user graph for sharing." << RESET << endl;
    }

    // Helper to find a user in the graph
    UserGraphNode* findUserNode(string username) {
        UserGraphNode* curr = head;
        while (curr) {
            if (curr->username == username) {
                return curr;
            }
            curr = curr->next;
        }
        return nullptr;
    }

    // Share a file with another user
    void shareFile(string ownerUsername, string receiverUsername, string filename, string permission)
    {
        UserGraphNode* ownerNode = findUserNode(ownerUsername);
        UserGraphNode* receiverNode = findUserNode(receiverUsername);

        if (!ownerNode) {
            cout << RED << "Owner user '" << ownerUsername << "' not found in graph." << RESET << endl;
            return;
        }
        if (!receiverNode) {
            cout << RED << "Receiver user '" << receiverUsername << "' not found in graph." << RESET << endl;
            return;
        }

        // Simple permission validation
        if (!(permission == "read" || permission == "write" || permission == "execute")) {
            cout << RED << "Invalid permission. Use 'read', 'write', or 'execute'." << RESET << endl;
            return;
        }

        // Add the shared file info to the owner's sharedFiles list (as they initiated the share)
        // Or you could add it to the receiver's list, depending on design.
        // For simplicity, let's add it to the owner's outbound shares.
        ownerNode->sharedFiles.emplace_back(receiverUsername, filename, permission);
        cout << GREEN << "File '" << filename << "' shared by " << ownerUsername
             << " with " << receiverUsername << " with permission: " << permission << RESET << endl;
    }

    // Display shared files for a user (files they have shared with others)
    void displaySharedFiles(string username)
    {
        UserGraphNode* curr = findUserNode(username);
        if (!curr)
        {
            cout << RED << "User '" << username << "' not found." << RESET << endl;
            return;
        }

        if (curr->sharedFiles.empty()) {
            cout << YELLOW << username << " has not shared any files." << RESET << endl;
            return;
        }

        cout << CYAN << "Files shared by " << username << ":" << RESET << endl;
        for (const auto& share : curr->sharedFiles)
        {
            cout << YELLOW << "-> Shared with: " << get<0>(share)
                 << ", File: " << get<1>(share)
                 << ", Permission: " << get<2>(share) << RESET << endl;
        }
    }

    // Display files shared *with* a user (inbound shares)
    void displayFilesSharedWithMe(string username) {
        if (!head) {
            cout << RED << "No users in the graph." << RESET << endl;
            return;
        }

        bool foundShares = false;
        cout << CYAN << "Files shared with " << username << ":" << RESET << endl;
        UserGraphNode* ownerNode = head;
        while (ownerNode) {
            for (const auto& share : ownerNode->sharedFiles) {
                if (get<0>(share) == username) { // If receiver is current user
                    cout << YELLOW << "<- From: " << ownerNode->username
                         << ", File: " << get<1>(share)
                         << ", Permission: " << get<2>(share) << RESET << endl;
                    foundShares = true;
                }
            }
            ownerNode = ownerNode->next;
        }

        if (!foundShares) {
            cout << YELLOW << "No files have been shared with " << username << "." << RESET << endl;
        }
    }
};

// Main File System class
class FileSystem
{
public:
    FolderNode* root;       // Root directory
    FolderNode* current;    // Current working directory
    HashTable metadata;     // Stores file metadata
    RecycleBin bin;         // Recycle bin for deleted files
    FileQueue recent;       // Recent files queue
    UserAuth auth;          // User authentication system
    UserGraph userGraph;    // User graph for file sharing
    FilePriorityHeap fileHeap; // Heap for managing file priorities
    string loggedInUser;    // Currently logged in user
    string loggedInUserRole; // Role of the currently logged in user

    // Constructor to initialize file system
    FileSystem() : fileHeap(100)
    {
        root = new FolderNode{ "root", nullptr, nullptr, nullptr, nullptr };
        current = root;
        loggedInUser = "";
        loggedInUserRole = "";
        auth.signup("admin", "admin123", "admin", "secret"); // Default admin user
        userGraph.addUser("admin"); // Add admin to graph
    }

    // Destructor to clean up the entire file system hierarchy
    ~FileSystem() {
        delete root; // Calls FolderNode's destructor, which recursively deletes everything
    }

    // Create a new folder in current directory
    void createFolder(string name)
    {
        // Check for duplication
        FolderNode* temp = current->child;
        while (temp) {
            if (temp->name == name) {
                cout << RED << "Folder '" << name << "' already exists in this directory." << RESET << endl;
                return;
            }
            temp = temp->sibling;
        }

        FolderNode* newFolder = new FolderNode{ name, current, nullptr, nullptr, nullptr };
        if (!current->child)
        {
            current->child = newFolder;
        }
        else
        {
            temp = current->child;
            while (temp->sibling)
            {
                temp = temp->sibling;
            }
            temp->sibling = newFolder;
        }
        cout << GREEN << "Folder created: " << name << RESET << endl;
    }

    // Create a new file with initial content
    void createFile(string name, string type, string content, int priority = 0)
    {
        // Permission check for creating files
        if (loggedInUserRole != "admin" && loggedInUserRole != "editor") {
             cout << RED << "Permission denied. Only admins and editors can create files." << RESET << endl;
             return;
        }

        FolderNode* targetFolder = current;
        if (current->child) {
            cout << "Available subfolders in current directory:" << endl;
            FolderNode* temp = current->child;
            int i = 1;
            while (temp) {
                cout << i << ". " << temp->name << endl;
                i++;
                temp = temp->sibling;
            }
            cout << "Enter subfolder name to create the file in, or type 'current' to use current directory: ";
            string folderNameChoice;
            getline(cin, folderNameChoice);

            if (!folderNameChoice.empty() && folderNameChoice != "current") {
                temp = current->child;
                while (temp) {
                    if (temp->name == folderNameChoice) {
                        targetFolder = temp;
                        break;
                    }
                    temp = temp->sibling;
                }
                if (!temp) {
                    cout << YELLOW << "Subfolder '" << folderNameChoice << "' not found. File will be created in the current directory." << RESET << endl;
                }
            }
        }

        // Check if file with same name already exists in target folder
        FileNode* existingFile = targetFolder->files;
        while (existingFile) {
            if (existingFile->name == name) {
                cout << YELLOW << "File '" << name << "' already exists. Adding a new version instead." << RESET << endl;
                // Add new version to existing file
                FileVersion* newVersion = new FileVersion{ content, nullptr, nullptr };
                FileVersion* ver = existingFile->versionHead;
                while (ver->next) {
                    ver = ver->next;
                }
                ver->next = newVersion;
                newVersion->prev = ver;
                cout << GREEN << "New version added for file '" << name << "'." << RESET << endl;
                recent.enqueue(name); // Mark as recently accessed
                return; // Exit as new version added
            }
            existingFile = existingFile->next;
        }

        // If file does not exist, create new file and its first version
        FileVersion* newVersion = new FileVersion{ content, nullptr, nullptr };
        FileNode* newFile = new FileNode{ name, type, loggedInUser, newVersion, nullptr, priority };

        if (!targetFolder->files)
        {
            targetFolder->files = newFile;
        }
        else
        {
            FileNode* temp = targetFolder->files;
            while (temp->next)
            {
                temp = temp->next;
            }
            temp->next = newFile;
        }

        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now);
        string dateStr(dt);
        metadata.insert(name, type, content.length(), loggedInUser, dateStr);
        recent.enqueue(name);
        fileHeap.insert(newFile);
        cout << GREEN << "File created: " << name << " in folder " << targetFolder->name << RESET << endl;
    }

    // List all folders in current directory
    void listFolders()
    {
        FolderNode* temp = current->child;
        if (!temp)
        {
            cout << RED << "No subfolders in current directory." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Subfolders in '" << current->name << "':" << RESET << endl;
            while (temp)
            {
                cout << YELLOW << temp->name << RESET << endl;
                temp = temp->sibling;
            }
        }
    }

    // List all files in current directory
    void listFiles()
    {
        FileNode* temp = current->files;
        if (!temp)
        {
            cout << RED << "No files in current directory." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Files in '" << current->name << "':" << RESET << endl;
            while (temp)
            {
                cout << YELLOW << temp->name << " (" << temp->type << ", Owner: " << temp->owner << ")" << RESET << endl;
                temp = temp->next;
            }
        }
    }

    // Find a file node in the current directory
    FileNode* findFileInCurrentDirectory(string name) {
        FileNode* temp = current->files;
        while (temp) {
            if (temp->name == name) {
                return temp;
            }
            temp = temp->next;
        }
        return nullptr;
    }

    // Display latest content of a file
    void readFile(string name)
    {
        FileNode* file = findFileInCurrentDirectory(name);
        if (!file)
        {
            cout << RED << "File not found in current directory." << RESET << endl;
            return;
        }

        // Access control check
        if (!file->canAccess(loggedInUserRole, "read")) { // Check if user has read permission
            cout << RED << "Permission denied to read file '" << name << "'." << RESET << endl;
            return;
        }

        FileVersion* ver = file->versionHead;
        while (ver->next) ver = ver->next; // Go to latest version
        cout << GREEN << "Latest Content of '" << name << "': " << ver->content << RESET << endl;
        recent.enqueue(name); // Mark as recently accessed
    }

    // Add new version to a file
    void updateFile(string name, string newContent)
    {
        FileNode* file = findFileInCurrentDirectory(name);
        if (!file)
        {
            cout << RED << "File not found in current directory." << RESET << endl;
            return;
        }

        // Access control check
        if (!file->canAccess(loggedInUserRole, "write")) {
            cout << RED << "Permission denied to write to file '" << name << "'." << RESET << endl;
            return;
        }

        FileVersion* ver = file->versionHead;
        while (ver->next) ver = ver->next; // Go to latest version
        FileVersion* newVer = new FileVersion{ newContent, ver, nullptr };
        ver->next = newVer;
        cout << GREEN << "File '" << name << "' updated with new version." << RESET << endl;
        recent.enqueue(name); // Mark as recently accessed
    }

    // Revert to previous version of a file
    void rollbackFile(string name)
    {
        FileNode* file = findFileInCurrentDirectory(name);
        if (!file) {
            cout << RED << "File not found in current directory." << RESET << endl;
            return;
        }

        // Access control check
        if (!file->canAccess(loggedInUserRole, "write")) { // Rollback is a write operation
            cout << RED << "Permission denied to rollback file '" << name << "'." << RESET << endl;
            return;
        }

        if (file->versionHead->next == nullptr) // Only one version exists
        {
            cout << RED << "No older version to rollback for file '" << name << "'." << RESET << endl;
            return;
        }

        FileVersion* ver = file->versionHead;
        while (ver->next) ver = ver->next; // Go to the latest version

        FileVersion* toDelete = ver; // This is the latest version we want to remove
        ver = ver->prev;             // Move back to the previous version
        ver->next = nullptr;         // Disconnect the latest version

        toDelete->prev = nullptr; // Disconnect the old prev pointer
        delete toDelete; // Delete the latest version (which recursively cleans up)

        cout << GREEN << "File '" << name << "' rolled back to previous version." << RESET << endl;
        recent.enqueue(name); // Mark as recently accessed
    }

    // Change current working directory
    void changeDirectory(string name)
    {
        if (name == ".." && current->parent)
        {
            current = current->parent;
            cout << GREEN << "Changed directory to parent." << RESET << endl;
            return;
        }
        if (name == "root") { // Special case to go to root
            current = root;
            cout << GREEN << "Changed directory to root." << RESET << endl;
            return;
        }

        FolderNode* temp = current->child;
        while (temp)
        {
            if (temp->name == name)
            {
                current = temp;
                cout << GREEN << "Changed directory to: " << name << RESET << endl;
                return;
            }
            temp = temp->sibling;
        }
        cout << RED << "Folder '" << name << "' not found in current directory." << RESET << endl;
    }

    // Delete a file (moves to recycle bin)
    void deleteFile(string name)
    {
        FileNode* curr = current->files;
        FileNode* prev = nullptr;
        while (curr)
        {
            if (curr->name == name)
            {
                // Access control check
                if (!curr->canAccess(loggedInUserRole, "write")) { // Deletion is a write operation
                    cout << RED << "Permission denied to delete file '" << name << "'." << RESET << endl;
                    return;
                }

                // Traverse to the latest version of the file
                FileVersion* ver = curr->versionHead;
                while (ver->next)
                {
                    ver = ver->next;
                }
                // Save the latest content to the recycle bin
                bin.push(name, ver->content);

                // Remove the file from the current directory
                if (prev)
                {
                    prev->next = curr->next;
                }
                else
                {
                    current->files = curr->next;
                }
                curr->next = nullptr; // Detach curr from the list to prevent deleting subsequent files
                delete curr; // This will call FileNode's destructor and recursively delete FileVersions
                metadata.remove(name); // Also remove from metadata hash table
                cout << GREEN << "File '" << name << "' successfully deleted and moved to Recycle Bin." << RESET << endl;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
        cout << RED << "File '" << name << "' not found in current directory." << RESET << endl;
    }

    // Delete a folder (and its contents)
    void deleteFolder(string name) {
        if (loggedInUserRole != "admin") {
            cout << RED << "Permission denied. Only admins can delete folders." << RESET << endl;
            return;
        }

        if (name == ".." || name == "root") {
            cout << RED << "Cannot delete special folders like '..' or 'root'." << RESET << endl;
            return;
        }

        FolderNode* curr = current->child;
        FolderNode* prev = nullptr;
        while (curr) {
            if (curr->name == name) {
                // Confirm deletion for safety
                cout << YELLOW << "WARNING: Deleting folder '" << name << "' will permanently delete all its contents. Are you sure? (yes/no): " << RESET;
                string confirmation;
                getline(cin, confirmation);
                if (confirmation != "yes") {
                    cout << BLUE << "Folder deletion cancelled." << RESET << endl;
                    return;
                }

                if (prev) {
                    prev->sibling = curr->sibling;
                } else {
                    current->child = curr->sibling;
                }
                curr->sibling = nullptr; // Detach from the sibling list
                delete curr; // Calls FolderNode's destructor, which recursively deletes all contained files and subfolders
                cout << GREEN << "Folder '" << name << "' and its contents permanently deleted." << RESET << endl;
                return;
            }
            prev = curr;
            curr = curr->sibling;
        }
        cout << RED << "Folder '" << name << "' not found in current directory." << RESET << endl;
    }


    // Print current directory path
    void printCurrentPath()
    {
        FolderNode* temp = current;
        string path = "";
        while (temp)
        {
            path = "/" + temp->name + path;
            temp = temp->parent;
        }
        cout << BLUE << "Current Path: " << path << RESET << endl;
    }

    // Display file metadata
    void viewMetadata(string name)
    {
        fileData* meta = metadata.search(name);
        if (!meta)
        {
            cout << RED << "Metadata not found for file '" << name << "'." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Metadata for '" << name << "':" << RESET << endl;
            cout << YELLOW << "Name: " << meta->name << "\nType: " << meta->type
                << "\nOwner: " << meta->owner << "\nSize: " << meta->size << " bytes"
                << "\nDate Created/Modified: " << meta->date << RESET << endl;
        }
    }

    // Share a file with another user
    void shareFileWithUser(string receiver, string filename, string permission)
    {
        // Check if the file exists and loggedInUser is its owner or has execute access (for sharing)
        FileNode* fileToShare = findFileInCurrentDirectory(filename);
        if (!fileToShare) {
            cout << RED << "File '" << filename << "' not found in current directory." << RESET << endl;
            return;
        }
        // Simplified check: only owner can share. More complex rules can be added.
        if (fileToShare->owner != loggedInUser) {
            cout << RED << "Permission denied. You are not the owner of file '" << filename << "'." << RESET << endl;
            return;
        }

        userGraph.shareFile(loggedInUser, receiver, filename, permission);
    }

    // Display files shared by the logged-in user
    void displaySharedFilesByMe()
    {
        userGraph.displaySharedFiles(loggedInUser);
    }

    // Display files shared with the logged-in user
    void displayFilesSharedWithMe() {
        userGraph.displayFilesSharedWithMe(loggedInUser);
    }


    // Display files by priority
    void displayFilesByPriority()
    {
        fileHeap.display();
    }
};

// Main program loop
int main()
{
    FileSystem fs;
    int choice;
    string name;
    string content;
    string type;
    string username;
    string password;
    string role;
    string secAns;
    string receiver;
    string permission;
    int priority;

    while (true) {
        cout << BOLD << MAGENTA << "--- Google Drive File System ---" << RESET << endl;
        if (!fs.loggedInUser.empty()) {
            cout << BOLD << CYAN << "Logged in as: " << fs.loggedInUser << " (" << fs.loggedInUserRole << ")" << RESET << endl;
            fs.printCurrentPath();
        } else {
            cout << YELLOW << "Please login or signup to use the file system." << RESET << endl;
        }
        cout << CYAN << "1. Signup" << RESET << endl;
        cout << CYAN << "2. Login" << RESET << endl;
        cout << CYAN << "3. Forgot Password" << RESET << endl;
        cout << CYAN << "4. Create Folder" << RESET << endl;
        cout << CYAN << "5. Create File" << RESET << endl;
        cout << CYAN << "6. List Folders" << RESET << endl;
        cout << CYAN << "7. List Files" << RESET << endl;
        cout << CYAN << "8. Change Directory" << RESET << endl;
        cout << CYAN << "9. Show Current Path" << RESET << endl;
        cout << CYAN << "10. Read File" << RESET << endl;
        cout << CYAN << "11. Update File (Add New Version)" << RESET << endl;
        cout << CYAN << "12. Rollback File to Previous Version" << RESET << endl;
        cout << CYAN << "13. Delete File (Move to Recycle Bin)" << RESET << endl;
        cout << CYAN << "14. Delete Folder" << RESET << endl; // New option
        cout << CYAN << "15. View File Metadata" << RESET << endl;
        cout << CYAN << "16. View Last Deleted File (Recycle Bin Top)" << RESET << endl;
        cout << CYAN << "17. Restore Last Deleted File (Recycle Bin Pop)" << RESET << endl; // New option
        cout << CYAN << "18. Display All Recycle Bin Contents" << RESET << endl; // New option
        cout << CYAN << "19. Recent Files (LRU)" << RESET << endl;
        cout << CYAN << "20. Share File" << RESET << endl;
        cout << CYAN << "21. View Files Shared By Me" << RESET << endl;
        cout << CYAN << "22. View Files Shared With Me" << RESET << endl; // New option
        cout << CYAN << "23. Display Files by Priority" << RESET << endl;
        cout << CYAN << "24. Logout" << RESET << endl;
        cout << CYAN << "0. Exit" << RESET << endl;

        while (true) {
            cout << GREEN << "Enter choice: " << RESET;
            if (cin >> choice) {
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer
                break;
            }
            else {
                cout << RED << "Invalid input. Please enter a number." << RESET << endl;
                cin.clear(); // Clear error flags
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
            }
        }

        if (choice == 1) // Signup
        {
            cout << "Enter Username: ";
            getline(cin, username);
            cout << "Enter Password: ";
            getline(cin, password);
            while (true) {
                cout << "Enter your role (admin, editor, viewer): ";
                getline(cin, role);
                if (role == "admin" || role == "editor" || role == "viewer") {
                    break;
                }
                else {
                    cout << RED << "Invalid role. Please enter admin, editor, or viewer." << RESET << endl;
                }
            }
            cout << "Enter your recovery code (e.g., your favorite color): ";
            getline(cin, secAns);
            fs.auth.signup(username, password, role, secAns);
            fs.userGraph.addUser(username); // Add user to the graph for sharing
            pauseAndClear();
        }
        else if (choice == 2) // Login
        {
            cout << "Enter Username: ";
            getline(cin, username);
            cout << "Enter Password: ";
            getline(cin, password);
            if (fs.auth.login(username, password))
            {
                fs.loggedInUser = username;
                fs.loggedInUserRole = fs.auth.getUserRole(username); // Get role after login
                cout << GREEN << "Welcome, " << username << " (" << fs.loggedInUserRole << ")!" << RESET << endl;
            }
            pauseAndClear();
        }
        else if (choice == 3) // Forgot Password
        {
            cout << "Enter Username: ";
            getline(cin, username);
            cout << "Enter Security Answer: ";
            getline(cin, secAns);
            fs.auth.forgot(username, secAns);
            pauseAndClear();
        }
        else if (choice == 0) // Exit
        {
            cout << "Exiting Google Drive File System. Goodbye!" << endl;
            break;
        }
        // Rest of the functionalities require login
        else if (fs.loggedInUser.empty())
        {
            cout << RED << "Permission denied. Please login first to perform this action." << RESET << endl;
            pauseAndClear();
        }
        else if (choice == 4) // Create Folder
        {
            cout << "Enter Folder name: ";
            getline(cin, name);
            fs.createFolder(name);
            pauseAndClear();
        }
        else if (choice == 5) // Create File
        {
            cout << "Enter File name: ";
            getline(cin, name);
            cout << "Enter File type (e.g., .txt, .pdf): ";
            getline(cin, type);
            cout << "Enter File content: ";
            getline(cin, content);
            cout << "Enter File priority (0-100, higher for more important): ";
            // Input validation for priority
            while (!(cin >> priority) || priority < 0 || priority > 100) {
                cout << RED << "Invalid priority. Please enter a number between 0 and 100: " << RESET;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer after numeric input
            fs.createFile(name, type, content, priority);
            pauseAndClear();
        }
        else if (choice == 6) // List Folders
        {
            fs.listFolders();
            pauseAndClear();
        }
        else if (choice == 7) // List Files
        {
            fs.listFiles();
            pauseAndClear();
        }
        else if (choice == 8) // Change Directory
        {
            cout << "Enter Folder name to change to (or '..' to go back, 'root' to go to root): ";
            getline(cin, name);
            fs.changeDirectory(name);
            pauseAndClear();
        }
        else if (choice == 9) // Show Current Path
        {
            fs.printCurrentPath();
            pauseAndClear();
        }
        else if (choice == 10) // Read File
        {
            cout << "Enter File name to read: ";
            getline(cin, name);
            fs.readFile(name);
            pauseAndClear();
        }
        else if (choice == 11) // Update File (Add New Version)
        {
            cout << "Enter File name to update: ";
            getline(cin, name);
            cout << "Enter New content: ";
            getline(cin, content);
            fs.updateFile(name, content);
            pauseAndClear();
        }
        else if (choice == 12) // Rollback File
        {
            cout << "Enter File name to rollback: ";
            getline(cin, name);
            fs.rollbackFile(name);
            pauseAndClear();
        }
        else if (choice == 13) // Delete File (Move to Recycle Bin)
        {
            cout << "Enter File name to delete: ";
            getline(cin, name);
            fs.deleteFile(name);
            pauseAndClear();
        }
        else if (choice == 14) // Delete Folder (New)
        {
            cout << "Enter Folder name to delete: ";
            getline(cin, name);
            fs.deleteFolder(name);
            pauseAndClear();
        }
        else if (choice == 15) // View Metadata
        {
            cout << "Enter File name to view metadata: ";
            getline(cin, name);
            fs.viewMetadata(name);
            pauseAndClear();
        }
        else if (choice == 16) // View Last Deleted File (Recycle Bin Top)
        {
            fs.bin.viewTop();
            pauseAndClear();
        }
        else if (choice == 17) // Restore Last Deleted File (Recycle Bin Pop) (New)
        {
            DeletedFile* restored = fs.bin.pop();
            if (restored) {
                // To fully restore, we need to recreate the file in the current folder.
                // For simplicity, we just show it's restored here.
                // In a real system, you'd ask where to restore and recreate FileNode.
                cout << GREEN << "Restored file '" << restored->name << "' with content: '" << restored->content << "'" << RESET << endl;
                delete restored; // Clean up the DeletedFile object after use
            }
            pauseAndClear();
        }
        else if (choice == 18) // Display All Recycle Bin Contents (New)
        {
            fs.bin.displayAll();
            pauseAndClear();
        }
        else if (choice == 19) // Recent Files (LRU)
        {
            fs.recent.display();
            pauseAndClear();
        }
        else if (choice == 20) // Share File
        {
            cout << "Enter Receiver Username: ";
            getline(cin, receiver);
            cout << "Enter File name to share (must be in current directory): ";
            getline(cin, name);
            cout << "Enter Permission (read/write/execute): ";
            getline(cin, permission);
            fs.shareFileWithUser(receiver, name, permission);
            pauseAndClear();
        }
        else if (choice == 21) // View Files Shared By Me
        {
            fs.displaySharedFilesByMe();
            pauseAndClear();
        }
        else if (choice == 22) // View Files Shared With Me (New)
        {
            fs.displayFilesSharedWithMe();
            pauseAndClear();
        }
        else if (choice == 23) // Display Files by Priority
        {
            fs.displayFilesByPriority();
            pauseAndClear();
        }
        else if (choice == 24) // Logout
        {
            fs.auth.logout(fs.loggedInUser);
            fs.loggedInUser = "";
            fs.loggedInUserRole = ""; // Clear role on logout
            cout << GREEN << "You have been logged out." << RESET << endl;
            pauseAndClear();
        }
        else {
            cout << RED << "Invalid choice. Please enter a valid option." << RESET << endl;
            pauseAndClear();
        }
    }
    // system("pause"); // This is Windows specific, might not work on other OS
    return 0;
}
