#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <limits>

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

// Structure to store different versions of a file in a doubly linked list
struct FileVersion
{
    string content;     // Content of this version
    FileVersion* prev;  // Pointer to previous version
    FileVersion* next;  // Pointer to next version
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
};

// Structure to store folder information in a tree structure
struct FolderNode
{
    string name;        // Folder name
    FolderNode* parent;  // Pointer to parent folder
    FolderNode* child;   // Pointer to first child folder
    FolderNode* sibling; // Pointer to next sibling folder
    FileNode* files;     // Pointer to files in this folder
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
};

// Structure for deleted files using stack implementation
struct DeletedFile
{
    string name;
    string content;
    DeletedFile* next;  // Pointer to next deleted file in stack
};

// Structure for recent files using queue implementation
struct RecentFile
{
    string name;
    RecentFile* next;  // Pointer to next recent file in queue
};

// Structure for user authentication using linked list
struct UserNode {
    string username;
    string password;
    string role; // Admin, Editor, Viewer
    string securityAnswer; // For password recovery
    string lastLogout;     // Timestamp of last logout
    UserNode* next;       // Pointer to next user
};

// Structure for graph-based user connections
struct UserGraphNode {
    string username;
    string sharedFiles[100]; // Array of shared files and permissions
    int sharedFilesCount;
    UserGraphNode* next;
};

// Heap structure for managing file priorities
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
        cout << CYAN << "Files in Heap by Priority:" << RESET << endl;
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
        fileData* node = new fileData{ key, type, owner, date, size, nullptr };

        if (!table[index])
        {
            table[index] = node;
        }
        else
        {
            fileData* curr = table[index];
            while (curr->next)
            {
                curr = curr->next;
            }
            curr->next = node;
        }
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
};

// Recycle Bin class using stack implementation
class RecycleBin
{
public:
    DeletedFile* top;  // Pointer to top of stack

    // Constructor to initialize recycle bin
    RecycleBin()
    {
        top = nullptr;
    }

    // Push a deleted file onto the stack
    void push(string name, string content)
    {
        top = new DeletedFile{ name, content, top };
    }

    // View the most recently deleted file
    void viewTop() {
        if (!top)
        {
            cout << RED << "Recycle Bin is empty" << RESET << endl;
        }
        else
        {
            cout << GREEN << "Last Deleted File: " << top->name << " - " << top->content << RESET << endl;
        }
    }
};

// Recent Files queue using FIFO implementation
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

    // Add file to end of queue
    void enqueue(string name)
    {
        if (size == capacity)
        {
            dequeue();
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
    }

    // Remove file from front of queue
    void dequeue()
    {
        if (!front)
        {
            return;
        }
        RecentFile* temp = front;
        front = front->next;
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
            cout << CYAN << "Recent Files:" << RESET << endl;
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

    // Register a new user
    void signup(string username, string password, string role, string secAns)
    {
        UserNode* curr = head;
        while (curr)
        {
            if (curr->username == username)
            {
                cout << RED << "Username already exists." << RESET << endl;
                return;
            }
            curr = curr->next;
        }
        head = new UserNode{ username, password, role, secAns, "", head };
        cout << GREEN << "Signup successful!" << RESET << endl;
    }

    // Authenticate user login
    bool login(string username, string password)
    {
        UserNode* curr = head;
        while (curr)
        {
            if (curr->username == username && curr->password == password)
            {
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
                cout << GREEN << "Password: " << curr->password << RESET << endl;
                return true;
            }
            curr = curr->next;
        }
        cout << RED << "Invalid username or answer." << RESET << endl;
        return false;
    }

    // Record logout time for user
    void logout(string username)
    {
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now);
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
};

// Graph-based user connections
class UserGraph
{
public:
    UserGraphNode* head;

    UserGraph()
    {
        head = nullptr;
    }

    // Add a new user to the graph
    void addUser(string username)
    {
        UserGraphNode* newUser = new UserGraphNode{ username, {}, 0, nullptr };
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
    }

    // Share a file with another user
    void shareFile(string owner, string receiver, string filename, string permission)
    {
        UserGraphNode* curr = head;
        while (curr)
        {
            if (curr->username == owner)
            {
                if (curr->sharedFilesCount < 100)
                {
                    curr->sharedFiles[curr->sharedFilesCount++] = receiver + ":" + filename + ":" + permission;
                    cout << GREEN << "File shared successfully." << RESET << endl;
                }
                else
                {
                    cout << RED << "Cannot share more files." << RESET << endl;
                }
                return;
            }
            curr = curr->next;
        }
        cout << RED << "User not found." << RESET << endl;
    }

    // Display shared files for a user
    void displaySharedFiles(string username)
    {
        UserGraphNode* curr = head;
        while (curr)
        {
            if (curr->username == username)
            {
                cout << CYAN << "Shared Files:" << RESET << endl;
                for (int i = 0; i < curr->sharedFilesCount; i++)
                {
                    cout << YELLOW << curr->sharedFiles[i] << RESET << endl;
                }
                return;
            }
            curr = curr->next;
        }
        cout << RED << "User not found." << RESET << endl;
    }
};

// Helper function to pause and clear screen
void pauseAndClear() {
    cout << "\nPress Enter to continue...";
    cin.ignore();
    system("cls");
}

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

    // Constructor to initialize file system
    FileSystem() : fileHeap(100)
    {
        root = new FolderNode{ "root", nullptr, nullptr, nullptr, nullptr };
        current = root;
        loggedInUser = "";
    }

    // Create a new folder in current directory
    void createFolder(string name)
    {
        FolderNode* newFolder = new FolderNode{ name, current, nullptr, nullptr, nullptr };
        if (!current->child)
        {
            current->child = newFolder;
        }
        else
        {
            FolderNode* temp = current->child;
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
        FolderNode* targetFolder = current;
        if (current->child) {
            cout << "Available folders:" << endl;
            FolderNode* temp = current->child;
            int i = 1;
            while (temp) {
                cout << i << ". " << temp->name << endl;
                i++;
                temp = temp->sibling;
            }
            cout << "Enter folder name to create the file in or press Enter to use current directory: ";
            string folderName;
            getline(cin, folderName);

            if (!folderName.empty()) {
                temp = current->child;
                while (temp) {
                    if (temp->name == folderName) {
                        targetFolder = temp;
                        break;
                    }
                    temp = temp->sibling;
                }
                if (!temp) {
                    cout << RED << "Folder not found. File will be created in the current directory." << RESET << endl;
                }
            }
        }

        FileVersion* newVersion = new FileVersion{ content, nullptr, nullptr };
        FileNode* newFile = new FileNode{ name, type, loggedInUser, newVersion, nullptr, priority };

        if (!targetFolder->files)
        {
            targetFolder->files = newFile;
        }
        else
        {
            FileNode* temp = targetFolder->files;
            while (temp)
            {
                if (temp->name == name)
                {
                    FileVersion* ver = temp->versionHead;
                    while (ver->next)
                    {
                        ver = ver->next;
                    }
                    ver->next = newVersion;
                    newVersion->prev = ver;
                    cout << GREEN << "New version added." << RESET << endl;
                    goto insert_metadata;
                }
                if (!temp->next)
                {
                    break;
                }
                temp = temp->next;
            }
            temp->next = newFile;
        }

    insert_metadata:
        time_t now = time(0);
        char dt[26];
        ctime_s(dt, sizeof(dt), &now);
        string dateStr(dt);
        metadata.insert(name, type, content.length(), loggedInUser, dateStr);
        recent.enqueue(name);
        fileHeap.insert(newFile);
    }

    // List all folders in current directory
    void listFolders()
    {
        FolderNode* temp = current->child;
        if (!temp)
        {
            cout << RED << "No folders." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Folders:" << RESET << endl;
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
            cout << RED << "No files." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Files:" << RESET << endl;
            while (temp)
            {
                cout << YELLOW << temp->name << " (" << temp->type << ")" << RESET << endl;
                temp = temp->next;
            }
        }
    }

    // Display latest content of a file
    void readFile(string name)
    {
        FileNode* temp = current->files;
        while (temp)
        {
            if (temp->name == name)
            {
                FileVersion* ver = temp->versionHead;
                while (ver->next) ver = ver->next;
                cout << GREEN << "Latest Content: " << ver->content << RESET << endl;
                return;
            }
            temp = temp->next;
        }
        cout << RED << "File not found." << RESET << endl;
    }

    // Add new version to a file
    void updateFile(string name, string newContent)
    {
        FileNode* temp = current->files;
        while (temp)
        {
            if (temp->name == name)
            {
                FileVersion* ver = temp->versionHead;
                while (ver->next) ver = ver->next;
                FileVersion* newVer = new FileVersion{ newContent, ver, nullptr };
                ver->next = newVer;
                cout << GREEN << "File updated with new version." << RESET << endl;
                return;
            }
            temp = temp->next;
        }
        cout << RED << "File not found." << RESET << endl;
    }

    // Revert to previous version of a file
    void rollbackFile(string name)
    {
        FileNode* temp = current->files;
        while (temp) {
            if (temp->name == name)
            {
                if (temp->versionHead->next == nullptr)
                {
                    cout << RED << "No older version to rollback." << RESET << endl;
                    return;
                }
                FileVersion* ver = temp->versionHead;
                while (ver->next) ver = ver->next;
                ver = ver->prev;
                delete ver->next;
                ver->next = nullptr;
                cout << GREEN << "Rolled back to previous version." << RESET << endl;
                return;
            }
            temp = temp->next;
        }
        cout << RED << "File not found." << RESET << endl;
    }

    // Change current working directory
    void changeDirectory(string name)
    {
        if (name == ".." && current->parent)
        {
            current = current->parent;
            return;
        }
        FolderNode* temp = current->child;
        while (temp)
        {
            if (temp->name == name)
            {
                current = temp;
                return;
            }
            temp = temp->sibling;
        }
        cout << RED << "Folder not found." << RESET << endl;
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
                delete curr;
                cout << GREEN << "File deleted." << RESET << endl;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
        cout << RED << "File not found." << RESET << endl;
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
            cout << RED << "Metadata not found." << RESET << endl;
        }
        else
        {
            cout << CYAN << "Name: " << meta->name << "\nType: " << meta->type
                << "\nOwner: " << meta->owner << "\nSize: " << meta->size
                << "\nDate: " << meta->date << RESET << endl;
        }
    }

    // Share a file with another user
    void shareFileWithUser(string receiver, string filename, string permission)
    {
        userGraph.shareFile(loggedInUser, receiver, filename, permission);
    }

    // Display shared files for the logged-in user
    void displaySharedFiles()
    {
        userGraph.displaySharedFiles(loggedInUser);
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
        cout << CYAN << "1. Signup" << RESET << endl;
        cout << CYAN << "2. Login" << RESET << endl;
        cout << CYAN << "3. Forgot Password" << RESET << endl;
        cout << CYAN << "4. Create Folder" << RESET << endl;
        cout << CYAN << "5. Create File" << RESET << endl;
        cout << CYAN << "6. List Folders" << RESET << endl;
        cout << CYAN << "7. List Files" << RESET << endl;
        cout << CYAN << "8. Change Directory" << RESET << endl;
        cout << CYAN << "9. Show Path" << RESET << endl;
        cout << CYAN << "10. Read File" << RESET << endl;
        cout << CYAN << "11. Update File" << RESET << endl;
        cout << CYAN << "12. Rollback File" << RESET << endl;
        cout << CYAN << "13. Delete File" << RESET << endl;
        cout << CYAN << "14. View Metadata" << RESET << endl;
        cout << CYAN << "15. Recycle Bin" << RESET << endl;
        cout << CYAN << "16. Recent Files" << RESET << endl;
        cout << CYAN << "17. Share File" << RESET << endl;
        cout << CYAN << "18. View Shared Files" << RESET << endl;
        cout << CYAN << "19. Display Files by Priority" << RESET << endl;
        cout << CYAN << "20. Logout" << RESET << endl;
        cout << CYAN << "0. Exit" << RESET << endl;

        while (true) {
            cout << GREEN << "Enter choice: " << RESET;
            if (cin >> choice) {
                cin.ignore();
                break;
            }
            else {
                cout << RED << "Invalid input. Please enter a number between 0 and 20." << RESET << endl;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
            }
        }

        if (choice == 1)
        {
            cout << "Username: ";
            getline(cin, username);
            cout << "Password: ";
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
            cout << "Enter your recovery code: ";
            getline(cin, secAns);
            fs.auth.signup(username, password, role, secAns);
            fs.userGraph.addUser(username);
            pauseAndClear();
        }
        else if (choice == 2)
        {
            cout << "Username: ";
            getline(cin, username);
            cout << "Password: ";
            getline(cin, password);
            if (fs.auth.login(username, password))
            {
                fs.loggedInUser = username;
                cout << GREEN << "Welcome, " << username << "!" << RESET << endl;
            }
            pauseAndClear();
        }
        else if (choice == 3)
        {
            cout << "Username: ";
            getline(cin, username);
            cout << "Security Answer: ";
            getline(cin, secAns);
            fs.auth.forgot(username, secAns);
            pauseAndClear();
        }
        else if (choice == 0)
        {
            cout << "Exiting..." << endl;
            break;
        }
        else if (fs.loggedInUser.empty())
        {
            cout << RED << "Please login first." << RESET << endl;
            pauseAndClear();
        }
        else if (choice == 4)
        {
            cout << "Folder name: ";
            getline(cin, name);
            fs.createFolder(name);
            pauseAndClear();
        }
        else if (choice == 5)
        {
            cout << "File name: ";
            getline(cin, name);
            cout << "File type: ";
            getline(cin, type);
            cout << "File content: ";
            getline(cin, content);
            cout << "File priority: ";
            cin >> priority;
            cin.ignore();
            fs.createFile(name, type, content, priority);
            pauseAndClear();
        }
        else if (choice == 6)
        {
            fs.listFolders();
            pauseAndClear();
        }
        else if (choice == 7)
        {
            fs.listFiles();
            pauseAndClear();
        }
        else if (choice == 8)
        {
            cout << "Folder name (or .. to go back): ";
            getline(cin, name);
            fs.changeDirectory(name);
            pauseAndClear();
        }
        else if (choice == 9)
        {
            fs.printCurrentPath();
            pauseAndClear();
        }
        else if (choice == 10)
        {
            cout << "File name: ";
            getline(cin, name);
            fs.readFile(name);
            pauseAndClear();
        }
        else if (choice == 11)
        {
            cout << "File name: ";
            getline(cin, name);
            cout << "New content: ";
            getline(cin, content);
            fs.updateFile(name, content);
            pauseAndClear();
        }
        else if (choice == 12)
        {
            cout << "File name: ";
            getline(cin, name);
            fs.rollbackFile(name);
            pauseAndClear();
        }
        else if (choice == 13)
        {
            cout << "File name: ";
            getline(cin, name);
            fs.deleteFile(name);
            pauseAndClear();
        }
        else if (choice == 14)
        {
            cout << "File name: ";
            getline(cin, name);
            fs.viewMetadata(name);
            pauseAndClear();
        }
        else if (choice == 15)
        {
            fs.bin.viewTop();
            pauseAndClear();
        }
        else if (choice == 16)
        {
            fs.recent.display();
            pauseAndClear();
        }
        else if (choice == 17)
        {
            cout << "Receiver Username: ";
            getline(cin, receiver);
            cout << "File name: ";
            getline(cin, name);
            cout << "Permission (read/write/execute): ";
            getline(cin, permission);
            fs.shareFileWithUser(receiver, name, permission);
            pauseAndClear();
        }
        else if (choice == 18)
        {
            fs.displaySharedFiles();
            pauseAndClear();
        }
        else if (choice == 19)
        {
            fs.displayFilesByPriority();
            pauseAndClear();
        }
        else if (choice == 20)
        {
            fs.auth.logout(fs.loggedInUser);
            fs.loggedInUser = "";
            cout << GREEN << "You have been logged out." << RESET << endl;
            pauseAndClear();
        }
        else {
            cout << RED << "Invalid choice." << RESET << endl;
            pauseAndClear();
        }
    }
    system("pause");
    return 0;
}

