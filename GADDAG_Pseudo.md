# GADDAG Data Structure for Scrabble AI

A **GADDAG** (Directed Acyclic Word Graph) is the standard data structure for Scrabble AIs. Unlike a normal Trie (which only goes left-to-right), a GADDAG allows you to start at *any* letter in a word and build outwards in both directions. This is crucial for hooking new words onto existing tiles on the board.

## 1. The Concept
To store the word **"CARE"**, we insert 4 different paths into the graph. We use a special separator symbol (let's use `+`) to mark the "turning point" between going backwards and going forwards.

1.  `C + A R E` (Start at C, go forward)
2.  `A C + R E` (Start at A, go back to C, then forward to R, E)
3.  `R A C + E` (Start at R, go back to A, C, then forward to E)
4.  `E R A C +` (Start at E, go back to R, A, C)

## 2. C++ Pseudo-Code

```cpp
// A Node in the GADDAG
struct Node {
    char letter;
    bool isEndOfWord; // True if this path forms a valid word
    
    // Edges to other nodes (could be a map or array[27])
    // 26 letters + 1 separator ('+')
    map<char, Node*> children; 
};

class GADDAG {
    Node* root;

public:
    GADDAG() { root = new Node(); }

    // Insert a word into the GADDAG
    void addWord(string word) {
        // We create a path for EVERY letter in the word acting as the "start"
        
        for (int i = 0; i < word.length(); i++) {
            // 1. Start with the prefix (letters BEFORE the start index, reversed)
            string path = "";
            
            // Add reversed prefix
            for (int j = i; j >= 0; j--) {
                path += word[j];
            }
            
            // 2. Add the separator
            path += '+';
            
            // 3. Add the suffix (letters AFTER the start index)
            for (int j = i + 1; j < word.length(); j++) {
                path += word[j];
            }
            
            // 4. Insert this specific path into the tree
            insertPath(path);
        }
    }

private:
    void insertPath(string path) {
        Node* current = root;
        for (char c : path) {
            if (current->children.find(c) == current->children.end()) {
                current->children[c] = new Node();
                current->children[c]->letter = c;
            }
            current = current->children[c];
        }
        current->isEndOfWord = true;
    }
};
```

## 3. How the AI Uses It
When the AI sees an 'A' on the board, it goes to the `root` of the GADDAG and follows the 'A' edge.
From there, it can branch:
*   Go to `C` (which means it's building "CA..." backwards)
*   Go to `+` (which means it's switching to forward mode)

This allows the AI to instantly find every single word that contains that specific 'A' on the board, without guessing.