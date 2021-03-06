/**
 * Contains all functions required to run a HFT limit order book.
 */
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "hftlob.h"

/**
 * Init functions for structs
 */

void 
initOrder(Order *order){
    order->tid = "NULL";
    order->buyOrSell = 0;
    order->shares = 0.0;
    order->limit = 0.0;
    order->entryTime = 0.0;
    order->eventTime = 0.0;
    order->nextOrder = NULL;
    order->prevOrder = NULL;
    order->parentLimit = NULL;
};

void
initLimit(Limit *limit){
    limit->limitPrice = 0.0;
    limit->size = 0.0;
    limit->totalVolume = 0.0;
    limit->orderCount = 0;
    limit->parent = NULL;
    limit->leftChild = NULL;
    limit->rightChild = NULL;
    limit->headOrder = NULL;
    limit->tailOrder = NULL;
};

void
initQueueItem(QueueItem *item){
    item->limit = NULL;
    item->previous = NULL;
}

void
initQueue(Queue *q){
    q->size = 0;
    q->head = NULL;
    q->tail = NULL;
}

/*
Functions for Order related operations
*/

int
pushOrder(Limit *limit, Order *newOrder){
    /**
     * Add an Order to a Limit structure at head.
     */
    if(limit->limitPrice != newOrder->limit){
        return 0;
    }
    newOrder->parentLimit = limit;
    newOrder->nextOrder = limit->headOrder;
    newOrder->prevOrder= NULL;


    if (limit->headOrder != NULL){
        limit->headOrder->prevOrder= newOrder;
    }
    else{
        limit->tailOrder = newOrder;
    };

    limit->headOrder = newOrder;
    limit->orderCount++;
    limit->size += newOrder->shares;
    limit->totalVolume += (newOrder->shares * limit->limitPrice);

    return 1;
}

Order*
popOrder(Limit *limit){
    /**
     * Pop the order at the tail of a Limit structure.
     */
    if (limit->tailOrder == NULL){
        return NULL;
    }

    Order *ptr_poppedOrder = limit->tailOrder;

    if (limit->tailOrder->prevOrder!= NULL){
        limit->tailOrder = limit->tailOrder->prevOrder;
        limit->tailOrder->nextOrder = NULL;
        limit->orderCount--;
        limit->size -= ptr_poppedOrder->shares;
        limit->totalVolume -= ptr_poppedOrder->shares * limit->limitPrice;
    }
    else{
        limit->headOrder = NULL;
        limit->tailOrder = NULL;
        limit->orderCount = 0;
        limit->size = 0;
        limit->totalVolume = 0;
    }

    return ptr_poppedOrder;
}

int
removeOrder(Order *order){
    /**
     * Remove the order from where it is at.
     */
    if(order->parentLimit->headOrder == order && order->parentLimit->tailOrder == order){
        /* Head and Tail are identical, set both to NULL and be done with it.*/
        order->parentLimit->headOrder = NULL;
        order->parentLimit->tailOrder = NULL;
    }
    else if(order->prevOrder != NULL && order->nextOrder != NULL){
        /* If Its in the middle, update reference for previous and next order and be done.*/
        order->prevOrder->nextOrder = order->nextOrder;
        order->nextOrder->prevOrder = order->prevOrder;
    }
    else if(order->nextOrder == NULL && order->parentLimit->tailOrder == order){
        /*This is the Tail - replace the tail with previous Order*/
        order->prevOrder->nextOrder = NULL;
        order->parentLimit->tailOrder = order->prevOrder;
    }
    else if(order->prevOrder == NULL && order->parentLimit->headOrder == order){
        /*This is the Head - replace the head with the next order*/
        order->nextOrder->prevOrder = NULL;
        order->parentLimit->headOrder = order->nextOrder;
    }
    else{
        return -1;
    }

    return 1;
}

/*
Limit-related data operations
*/

Limit*
createRoot(void){
    /**
     * Create a Limit structure as root and return a ptr to it.
     */
    Limit *ptr_limit = malloc(sizeof(Limit));
    initLimit(ptr_limit);
    ptr_limit->limitPrice = -INFINITY;
    return ptr_limit;
}

int
addNewLimit(Limit *root, Limit *limit){
    /**
     * Add a new Limit struct to the given limit tree.
     *
     * Asserts that the limit does not yet exist.
     * Also sets left and right child to NULL.
     */
    if(limitExists(root, limit) == 1){
        return 0;
    }
    limit->leftChild = NULL;
    limit->rightChild = NULL;


    Limit *currentLimit = root;
    Limit *child;
    while(1){
        if(currentLimit->limitPrice < limit->limitPrice){
            if(currentLimit->rightChild == NULL){
                currentLimit->rightChild = limit;
                limit->parent = currentLimit;
                return 1;
            }
            else{
                currentLimit = currentLimit->rightChild;
            }
        }
        else if (currentLimit->limitPrice > limit->limitPrice){
            if(currentLimit->leftChild == NULL){
                currentLimit->leftChild = limit;
                limit->parent = currentLimit;
                return 1;
            }
            else{
                currentLimit = currentLimit->leftChild;
            }
        }
        else{ /*If its neither greater than nor smalle then it must be equal, and hence exist.*/
            break;
        }
        continue;

    }
    return 0;
}

void
replaceLimitInParent(Limit *limit, Limit *newLimit) {
    /**
     * Pop out the given limit and replace all pointers to it from limit->parent
     * to point to the newLimit.
     *
     * Serve all cases : Node has no children, node has one child, node has two children.
     *
     * Python Reference code here:
     *     https://en.wikipedia.org/wiki/Binary_search_tree#Deletion
     */

    if(!limitIsRoot(limit)){
        if(limit==limit->parent->leftChild && !limitIsRoot(limit->parent)){
            limit->parent->leftChild = newLimit;
        }
        else{
            limit->parent->rightChild = newLimit;
        }
    }
    if(newLimit!=NULL){
        newLimit->parent = limit->parent;
    }
}

int
removeLimit(Limit *limit){
    /**
     * Remove the given limit from the tree it belongs to.
     *
     * This assumes it IS part of a limit tree.
     *
     * Python Reference code here:
     *     https://en.wikipedia.org/wiki/Binary_search_tree#Deletion
     */
    if(!hasGrandpa(limit) && limitIsRoot(limit)){
        return 0;
    }

    Limit *ptr_successor = limit;
    if(limit->leftChild != NULL && limit->rightChild != NULL){
        /*Limit has two children*/
        ptr_successor = getMinimumLimit(limit->rightChild);
        Limit *parent = ptr_successor->parent;
        Limit *leftChild = ptr_successor->rightChild;
        Limit *rightChild = ptr_successor->leftChild;


        if(limit->leftChild != ptr_successor){
            ptr_successor->leftChild = limit->leftChild;
        }
        else{
            ptr_successor->leftChild = NULL;
        }

        if(limit->rightChild != ptr_successor){
            ptr_successor->rightChild = limit->rightChild;
        }
        else{
            ptr_successor->rightChild = NULL;
        }
        limit->leftChild = leftChild;
        limit->rightChild = rightChild;
        ptr_successor->parent = limit->parent;
        if(ptr_successor->parent->rightChild==limit){
            ptr_successor->parent->rightChild = ptr_successor;
        }
        else if(ptr_successor->parent->leftChild==limit){
            ptr_successor->parent->leftChild = ptr_successor;
        }
        limit->parent = parent;

        removeLimit(limit);
    }
    else if(limit->leftChild != NULL && limit->rightChild == NULL){
        /*Limit has only left child*/
        replaceLimitInParent(limit, limit->leftChild);
    }
    else if(limit->leftChild != NULL && limit->rightChild == NULL){
        /*Limit has only left child*/
        replaceLimitInParent(limit, limit->rightChild);
    }
    else{
        /*Limit has no children*/
        replaceLimitInParent(limit, NULL);
    }
    return 1;
}


/*
Limit-related BST rotation functions.
*/

void
balanceBranch(Limit *limit) {
    /**
     * Balance the nodes of the given branch of Limit structs.
     *
     * Asserts that limit has a height of at least 2.
     */
    assert(getHeight(limit) < 2);

    int balanceFactor = getBalanceFactor(limit);
    if(balanceFactor > 1){
        /*Right is heavier.*/
        balanceFactor = getBalanceFactor(limit->rightChild);
        if(balanceFactor < 0){
            rotateRightLeft(limit);
        }
        else if(balanceFactor > 0){
            rotateRightRight(limit);
        }
    }
    else if(balanceFactor < -1){
        /*Left is heavier.*/
        balanceFactor = getBalanceFactor(limit->leftChild);
        if(balanceFactor < 0){
            rotateLeftLeft(limit);
        }
        else if(balanceFactor > 0){
            rotateLeftRight(limit);
        }
    }
    else{/*Everything is fine, do nothing*/}
}

void
rotateLeftLeft(Limit *limit) {
    /**
     * Rotate tree nodes for LL Case
     * Reference:
     *     https://en.wikipedia.org/wiki/File:Tree_Rebalancing.gif
     */
    Limit *child = limit->leftChild;
    if(limitIsRoot(limit->parent)==1 || limit->limitPrice > limit->parent->limitPrice){
        limit->parent->rightChild = child;
    }
    else{
        limit->parent->leftChild = child;
    }
    child->parent = limit->parent;
    limit->parent = child;
    child->rightChild = limit;
    limit->leftChild = child->rightChild;
    return;
}

void
rotateLeftRight(Limit *limit) {
    /**
     * Rotate tree nodes for LR Case
     * Reference:
     *     https://en.wikipedia.org/wiki/File:Tree_Rebalancing.gif
     */
    Limit *child = limit->leftChild;
    Limit *grandChild = limit->leftChild->rightChild;
    child->parent = grandChild;
    grandChild->parent = limit;
    child->rightChild = grandChild->leftChild;
    limit->leftChild = grandChild;
    grandChild->leftChild = child;
    rotateLeftLeft(limit);
    return;
}

void
rotateRightRight(Limit *limit){
    /**
     * Rotate tree nodes for RR Case
     * Reference:
     *     https://en.wikipedia.org/wiki/File:Tree_Rebalancing.gif
     */
    Limit *child = limit->rightChild;
    if(limitIsRoot(limit->parent)==1 || limit->limitPrice > limit->parent->limitPrice){
        limit->parent->rightChild = child;
    }
    else{
        limit->parent->leftChild = child;
    }
    child->parent = limit->parent;
    limit->parent = child;
    child->leftChild = limit;
    limit->rightChild = child->leftChild;
    return;
}

void
rotateRightLeft(Limit *limit){
    /**
     * Rotate tree nodes for RL Case.
     *
     * Reference:
     *     https://en.wikipedia.org/wiki/File:Tree_Rebalancing.gif
     */
    Limit *child = limit->rightChild;
    Limit *grandChild = limit->rightChild->leftChild;
    child->parent = grandChild;
    grandChild->parent = limit;
    child->leftChild = grandChild->rightChild;
    limit->rightChild = grandChild;
    grandChild->rightChild = child;
    rotateRightRight(limit);
    return;
}


/*
Limit-related convenience functions to query attributes
about a Limit struct.

These are mainly used to make important code parts more readable,
by being more descriptive.
*/

int
limitExists(Limit *root, Limit *limit){
    /**
     * Check if the given price level (value) exists in the
     * given limit tree (root).
     */
    if(root->parent == NULL && root->rightChild == NULL){
        return 0;
    }
    Limit *currentLimit = root;
    while(currentLimit->limitPrice != limit->limitPrice){
        if(currentLimit->leftChild == NULL && currentLimit->rightChild==NULL){
            return 0;
        }
        else {
            if(currentLimit->rightChild != NULL && currentLimit->limitPrice < limit->limitPrice){
                currentLimit = currentLimit->rightChild;
            }
            else if(currentLimit->leftChild != NULL && currentLimit->limitPrice > limit->limitPrice){
                currentLimit = currentLimit->leftChild;
            }
            else{
                return -1;
            }
            continue;
        }
    }
    return 1;
}

int
limitIsRoot(Limit *limit){
    /**
     * Check if the given limit is the root of the limit tree.
     */
    if(limit->parent==NULL){
        return 1;
    }
    else{
        return 0;
    }
}

int
hasGrandpa(Limit *limit){
    /**
     * Check if there is a parent to the passed limit's parent.
     */
    if(limit->parent != NULL && limit->parent->parent != NULL){
        return 1;
    }
    else{
        return 0;
    }
}

Limit*
getGrandpa(Limit *limit){
    /**
     * Return the limit's parent parent.
     */
    if(hasGrandpa(limit)){
        return limit->parent->parent;
    }
    return NULL;
}

Limit*
getMinimumLimit(Limit *limit){
    /**
     * Return the left-most limit struct for the given limit
     * tree / branch.
     */
    Limit *ptr_minimum;
    if(limitIsRoot(limit)){
        ptr_minimum = limit->rightChild;
    }
    else{
        ptr_minimum = limit;
    }

    while(ptr_minimum->leftChild != NULL){
        ptr_minimum = ptr_minimum->leftChild;
    }
    return (ptr_minimum);
}

Limit*
getMaximumLimit(Limit *limit){
    /**
     * Return the right-most limit struct for the given limit
     * tree / branch.
     */
    Limit *ptr_maximum = limit;
    while(ptr_maximum->rightChild != NULL){
        ptr_maximum = ptr_maximum->rightChild;
    }
    return (ptr_maximum);
}

int
getHeight(Limit *limit){
    /**
     * Calculate the height of the limits under the passed limit non-recursively.
     */

     if(limit == NULL){
        return -1;
     }

    int height = -1; /*Set to -1; if limit has no children, this will end up being 0*/
    int qsize = 0;

    Queue *ptr_queue = malloc(sizeof(Queue));
    initQueue(ptr_queue);
    Limit *ptr_current;
    pushToQueue(ptr_queue, limit);
    while(1){
        qsize = ptr_queue->size;
        if(qsize == 0){
            break;
        }
        height++;
        while(qsize > 0){
            ptr_current = popFromQueue(ptr_queue);
            if(ptr_current->leftChild != NULL){
                pushToQueue(ptr_queue, ptr_current->leftChild);
            }
            if(ptr_current->rightChild != NULL){
                pushToQueue(ptr_queue, ptr_current->rightChild);
            }
            qsize--;
        }
    }
    free(ptr_queue);
    return height;
}

int
getBalanceFactor(Limit *limit){
    /**
     * Calculate the balance factor of the passed limit, by
     * subtracting the left children's height from the right children's
     * height.
     */
    int leftHeight = -1;
    int rightHeight = -1;
    if(limit->rightChild!=NULL){
        rightHeight = getHeight(limit->rightChild);
    }

    if(limit->leftChild!=NULL){
        leftHeight = getHeight(limit->leftChild);
    }
    return rightHeight - leftHeight;
}

void
copyLimit(Limit *ptr_src, Limit *ptr_tar){
    /**
     * Copy the values of src limit to tar limit.
     */
    ptr_tar->limitPrice = ptr_src->limitPrice;
    ptr_tar->size = ptr_src->size;
    ptr_tar->totalVolume = ptr_src->totalVolume;
    ptr_tar->orderCount = ptr_src->orderCount;
    ptr_tar->headOrder = ptr_src->headOrder;
    ptr_tar->tailOrder = ptr_src->tailOrder;
    Order *ptr_order = ptr_tar->headOrder;

    while(ptr_order != NULL){
        ptr_order->parentLimit = ptr_tar;
        if(ptr_order != NULL){
            ptr_order = ptr_order->nextOrder;
        }
    }
}

