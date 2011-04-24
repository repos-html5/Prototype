/********************************************************************
 *             a  A                                                        
 *            AM\/MA                                                         
 *           (MA:MMD                                                         
 *            :: VD
 *           ::  º                                                         
 *          ::                                                              
 *         ::   **      .A$MMMMND   AMMMD     AMMM6    MMMM  MMMM6             
 +       6::Z. TMMM    MMMMMMMMMDA   VMMMD   AMMM6     MMMMMMMMM6            
 *      6M:AMMJMMOD     V     MMMA    VMMMD AMMM6      MMMMMMM6              
 *      ::  TMMTMC         ___MMMM     VMMMMMMM6       MMMM                   
 *     MMM  TMMMTTM,     AMMMMMMMM      VMMMMM6        MMMM                  
 *    :: MM TMMTMMMD    MMMMMMMMMM       MMMMMM        MMMM                   
 *   ::   MMMTTMMM6    MMMMMMMMMMM      AMMMMMMD       MMMM                   
 *  :.     MMMMMM6    MMMM    MMMM     AMMMMMMMMD      MMMM                   
 *         TTMMT      MMMM    MMMM    AMMM6  MMMMD     MMMM                   
 *        TMMMM8       MMMMMMMMMMM   AMMM6    MMMMD    MMMM                   
 *       TMMMMMM$       MMMM6 MMMM  AMMM6      MMMMD   MMMM                   
 *      TMMM MMMM                                                           
 *     TMMM  .MMM                                         
 *     TMM   .MMD       ARBITRARY·······XML········RENDERING                           
 *     TMM    MMA       ====================================                              
 *     TMN    MM                               
 *      MN    ZM                       
 *            MM,
 *
 * 
 *      AUTHORS: Miro Keller
 *      
 *      COPYRIGHT: ©2011 - All Rights Reserved
 *
 *      LICENSE: see License.txt file
 *
 *      WEB: http://axr.vg
 *
 *      THIS CODE AND INFORMATION ARE PROVIDED "AS IS"
 *      WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *      IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
 *      FITNESS FOR A PARTICULAR PURPOSE.
 *
 ********************************************************************
 *
 *      FILE INFORMATION:
 *      =================
 *      Last changed: 2011/04/24
 *      HSS version: 1.0
 *      Core version: 0.3
 *      Revision: 13
 *
 ********************************************************************/

#include "HSSDisplayObject.h"
#include "../../axr/AXRDebugging.h"
#include <math.h>
#include "HSSParserNode.h"
#include "HSSExpression.h"
#include <boost/pointer_cast.hpp>
#include <cairo/cairo.h>
#include "../parsing/HSSExpression.h"
#include "../parsing/HSSConstants.h"
#include "HSSContainer.h"

using namespace AXR;

HSSDisplayObject::HSSDisplayObject()
: HSSObject()
{
    std_log1(std::string("creating annonymous display object"));
    this->type = HSSObjectTypeDisplayObject;
    
    this->_isDirty= true;
    this->_needsRereadRules = false;
    this->_needsSurface = true;
    this->backgroundSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    this->foregroundSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    this->bordersSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    
    x = y = width = height = anchorX = anchorY = alignX = alignY = 0.;
    drawIndex = 0;
    tabIndex = zoomFactor = 1;
    flow = visible = true;
    does_float = false;
    
    elementName = std::string();
    contentText = std::string();
    
    observedWidth = observedHeight = NULL;
}

HSSDisplayObject::HSSDisplayObject(std::string name)
: HSSObject(name)
{
    std_log1(std::string("creating display object with name ").append(name));   
    this->type = HSSObjectTypeDisplayObject;
    
    this->_isDirty = true;
    this->_needsRereadRules = false;
    this->_needsSurface = true;
    this->backgroundSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    this->foregroundSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    this->bordersSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    
    x = y = width = height = anchorX = anchorY = alignX = alignY = 0.;
    drawIndex = 0;
    tabIndex = zoomFactor = 1;
    flow = visible = true;
    does_float = false;
    
    elementName = std::string();
    contentText = std::string();
    
    observedWidth = observedHeight = NULL;
}

HSSDisplayObject::~HSSDisplayObject()
{
    std_log1(std::string("destroying display object with name ").append(this->name));
    cairo_surface_destroy(this->backgroundSurface);
    cairo_surface_destroy(this->foregroundSurface);
    cairo_surface_destroy(this->bordersSurface);
}

std::string HSSDisplayObject::toString()
{
    if (this->isNamed()) {
        return std::string("HSSDisplayObject: ").append(this->name);
    } else {
        return "Annonymous HSSDisplayObject";
    }
}

std::string HSSDisplayObject::defaultObjectType(std::string property)
{
    if (property == "margin"){
        return "margin";
    } else if (property == "border"){
        return "lineBorder";
    } else if (property == "transform"){
        return "rotate";
    } else if (property == "effects"){
        return "shadow";
    } else if (property == "animation") {
        return "transition";
    } else if (property == "behavior") {
        return "click";
    } else if (property == "mask") {
        return "image";
    } else {
        return HSSObject::defaultObjectType(property);
    }
}

bool HSSDisplayObject::isKeyword(std::string value, std::string property)
{
    if (value == "center"){
        if (   property == "anchorX"
            || property == "anchorY"
            || property == "alignX"
            || property == "alignY"
            ) {
            return true;
        }
    } else if (value == "yes" || value == "no"){
        if (property == "flow") {
            return true;
        }
    }
    
    //if we reached this far, let the superclass handle it
    return HSSObject::isKeyword(value, property);
}

boost::shared_ptr<HSSContainer> HSSDisplayObject::getParent()
{
    boost::shared_ptr<HSSContainer> parent = this->parent.lock();
    return parent;
}

void HSSDisplayObject::setParent(boost::shared_ptr<HSSContainer> parent)
{
    this->parent = parentPointer(parent);
}


void HSSDisplayObject::attributesAdd(std::string name, std::string value)
{
    this->attributes[name] = value;
}

void HSSDisplayObject::attributesRemove(std::string name)
{
    this->attributes.erase(name);
}


std::string HSSDisplayObject::getElementName()
{
    return this->elementName;
}

void HSSDisplayObject::setElementName(std::string newName)
{
    this->elementName = newName;
}

//rules

void HSSDisplayObject::rulesAdd(HSSRule::p newRule)
{
    this->rules.push_back(newRule);
}

HSSRule::p HSSDisplayObject::rulesGet(unsigned index)
{
    return this->rules[index];
}

void HSSDisplayObject::rulesRemove(unsigned index)
{
    this->rules.erase(this->rules.begin()+index);
}

void HSSDisplayObject::rulesRemoveLast()
{
    this->rules.pop_back();
}

const int HSSDisplayObject::rulesSize()
{
    return this->rules.size();
}

void HSSDisplayObject::readDefinitionObjects()
{
//    if(this->_needsRereadRules){
        unsigned i, j;
    
        this->setDefaults();
        
        std::string propertyName;
        for (i=0; i<this->rules.size(); i++) {
            HSSRule::p& rule = this->rules[i];
            
            for (j=0; j<rule->propertiesSize(); j++) {
                HSSPropertyDefinition::p& propertyDefinition = rule->propertiesGet(j);
                propertyName = propertyDefinition->getName();
                
                if(propertyName == "width"){
                    //store a copy of the value
                    this->setDWidth(propertyDefinition->getValue());
                    
                } else if(propertyName == "height"){
                    //store a copy of the value
                    this->setDHeight(propertyDefinition->getValue());
                }
            }
        }
        
        this->_needsRereadRules = false;
//    }
}

void HSSDisplayObject::setNeedsRereadRules(bool value)
{
    this->_needsRereadRules = value;
}

bool HSSDisplayObject::needsRereadRules()
{
    return this->_needsRereadRules;
}

void HSSDisplayObject::recursiveReadDefinitionObjects()
{
    this->readDefinitionObjects();
}

void HSSDisplayObject::regenerateSurfaces()
{
//    if(this->_needsSurface){
        cairo_surface_destroy(this->backgroundSurface);
        cairo_surface_destroy(this->foregroundSurface);
        cairo_surface_destroy(this->bordersSurface);
        this->backgroundSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(this->width), ceil(this->height));
        this->foregroundSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(this->width), ceil(this->height));
        this->bordersSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, ceil(this->width), ceil(this->height));
        this->setDirty(true);
        this->_needsSurface = false;
        fprintf(stderr, "created a new surface width: %f, height: %f\n", this->width, this->height);
//    }
}

void HSSDisplayObject::recursiveRegenerateSurfaces()
{
    this->regenerateSurfaces();
}


void HSSDisplayObject::setNeedsSurface(bool value)
{
    this->_needsSurface = value;
}

bool HSSDisplayObject::needsSurface()
{
    return this->_needsSurface;
}


void HSSDisplayObject::setDirty(bool value)
{
    this->_isDirty = value;
}

bool HSSDisplayObject::isDirty()
{
    return this->_isDirty;
}

void HSSDisplayObject::draw(cairo_t * cairo)
{
    std_log1("drawing "+this->elementName);
    if(this->_needsRereadRules){
        std_log1("rereading rules of "+this->elementName);
        this->readDefinitionObjects();
    }
    if(this->_needsSurface){
        std_log1("regenerating surfaces of "+this->elementName);
        this->regenerateSurfaces();
    }
    if(this->_isDirty){
        std_log1("redrawing contents of "+this->elementName);
        this->_isDirty = false;
        cairo_t * cairo = cairo_create(this->foregroundSurface);
        cairo_rectangle(cairo, 0., 0., this->width, this->height);
        cairo_set_source_rgb(cairo, 0.8,0.8,0.8);
        cairo_fill_preserve(cairo);
        cairo_set_source_rgb(cairo, 0,0,0);
        cairo_stroke(cairo);
        cairo_destroy(cairo);
    }
    
    cairo_set_source_surface(cairo, this->foregroundSurface, 0, 0);
    cairo_paint(cairo);
}

void HSSDisplayObject::recursiveDraw(cairo_t * cairo)
{
    this->draw(cairo);
}


//callbacks
void HSSDisplayObject::propertyChanged(HSSObservableProperty property, void* data)
{
    switch (property) {
        //width
        case HSSObservablePropertyWidth:
        {
            HSSParserNodeType nodeType = this->dWidth->getType();
            switch (nodeType) {
                case HSSParserNodeTypePercentageConstant:
                {
                    HSSPercentageConstant::p widthValue = boost::static_pointer_cast<HSSPercentageConstant>(this->dWidth);
                    this->width = widthValue->getValue(*(double*)data);
                    break;
                }
                    
                default:
                    break;
            }
            
            this->notifyObservers(HSSObservablePropertyWidth, &this->width);
            this->setNeedsSurface(true);
            this->setDirty(true);
            
            break;
        }
        
        //height
        case HSSObservablePropertyHeight:
        {
            HSSParserNodeType nodeType = this->dHeight->getType();
            switch (nodeType) {
                case HSSParserNodeTypePercentageConstant:
                {
                    HSSPercentageConstant::p heightValue = boost::static_pointer_cast<HSSPercentageConstant>(this->dHeight);
                    this->height = heightValue->getValue(*(double*)data);
                    break;
                }
                    
                default:
                    break;
            }
            
            this->notifyObservers(HSSObservablePropertyHeight, &this->height);
            this->setNeedsSurface(true);
            this->setDirty(true);
            
            break;
        }
            
        default:
            break;
    }
}


//width
HSSParserNode::p HSSDisplayObject::getDWidth()  {   return this->dWidth; }
void HSSDisplayObject::setDWidth(HSSParserNode::p value)
{
    std_log1("setting width of "+this->name+":");
    this->dWidth = value;
    HSSParserNodeType nodeType = value->getType();
    
    if(observedWidth != NULL)
    {
        //std_log1("#################should unsubscribe observer here#########################");
        observedWidth->removeObserver(HSSObservablePropertyWidth, this);
    }
    
    switch (nodeType) {
        case HSSParserNodeTypeNumberConstant:
        {
            HSSNumberConstant::p widthValue = boost::static_pointer_cast<HSSNumberConstant>(value);
            this->width = widthValue->getValue();
            break;
        }
            
        case HSSParserNodeTypePercentageConstant:
        {
            HSSPercentageConstant::p widthValue = boost::static_pointer_cast<HSSPercentageConstant>(value);
            HSSContainer::p parentContainer = this->getParent();
            if(parentContainer){
                this->width = widthValue->getValue(parentContainer->width);
                parentContainer->observe(HSSObservablePropertyWidth, this, new HSSValueChangedCallback(this, &HSSObject::propertyChanged));
                this->observedWidth = parentContainer.get();
                
            } else {
                this->width = widthValue->getValue(this->width);
            }
            break;
        }
            
        case HSSParserNodeTypeExpression:
        {
            HSSExpression::p widthExpression = boost::static_pointer_cast<HSSExpression>(value);
            HSSContainer::p parentContainer = this->getParent();
            if(parentContainer){
                this->width = widthExpression->evaluate(parentContainer->width, parentContainer->getChildren());
            } else {
                this->width = widthExpression->evaluate(this->width);
            }
            break; 
        }
            
        case HSSParserNodeTypeKeywordConstant:
            
            break;
            
        default:
            throw "unknown parser node type while setting dWidth";
            break;
    }
    
    fprintf(stderr, "%f\n", this->width);
    this->notifyObservers(HSSObservablePropertyWidth, &this->width);
    this->setNeedsSurface(true);
    this->setDirty(true);
}

//height
HSSParserNode::p HSSDisplayObject::getDHeight() { return this->dHeight; }
void HSSDisplayObject::setDHeight(HSSParserNode::p value)
{
    std_log1("setting height of "+this->name+":");
    this->dHeight = value;
    HSSParserNodeType nodeType = value->getType();
    
    if(observedHeight != NULL)
    {
        observedHeight->removeObserver(HSSObservablePropertyHeight, this);
    }
    
    switch (nodeType) {
        case HSSParserNodeTypeNumberConstant:
        {
            HSSNumberConstant::p heightValue = boost::static_pointer_cast<HSSNumberConstant>(value);
            this->height = heightValue->getValue();
            break;
        }
            
        case HSSParserNodeTypePercentageConstant:
        {
            HSSPercentageConstant::p heightValue = boost::static_pointer_cast<HSSPercentageConstant>(value);
            HSSContainer::p parentContainer = this->getParent();
            if(parentContainer){
                this->height = heightValue->getValue(parentContainer->height);
                parentContainer->observe(HSSObservablePropertyHeight, this, new HSSValueChangedCallback(this, &HSSObject::propertyChanged));
                this->observedHeight = parentContainer.get();
            }
            break;
        }
        
        case HSSParserNodeTypeExpression:
        {
            HSSExpression::p heightExpression = boost::static_pointer_cast<HSSExpression>(value);
            HSSContainer::p parentContainer = this->getParent();
            if(parentContainer){
                this->height = heightExpression->evaluate(parentContainer->height);
            } else {
                this->height = heightExpression->evaluate(this->height);
            }
            break; 
        }
            
        case HSSParserNodeTypeKeywordConstant:
            
            break;
            
        default:
            throw "unknown parser node type while setting dHeight";
            break;
    }
    
    fprintf(stderr, "%f\n", this->height);
    this->notifyObservers(HSSObservablePropertyHeight, &this->height);
    this->setNeedsSurface(true);
    this->setDirty(true);
}
//anchorX
HSSParserNode::p HSSDisplayObject::getDAnchorX() { return this->dAnchorX; }
void HSSDisplayObject::setDAnchorX(HSSParserNode::p value)
{
    this->dAnchorX = value;
}
//anchorY
HSSParserNode::p HSSDisplayObject::getDAnchorY() { return this->dAnchorY; }
void HSSDisplayObject::setDAnchorY(HSSParserNode::p value)
{
    this->dAnchorY = value;
}
//flow
HSSParserNode::p HSSDisplayObject::getDFlow() { return this->dFlow; }
void HSSDisplayObject::setDFlow(HSSParserNode::p value)
{
    this->dFlow = value;
}
//alignX
HSSParserNode::p HSSDisplayObject::getDAlignX() { return this->dAlignX; }
void HSSDisplayObject::setDAlignX(HSSParserNode::p value)
{
    this->dAlignX = value;
}
//alignY
HSSParserNode::p HSSDisplayObject::getDAlignY() { return this->dAlignY; }
void HSSDisplayObject::setDAlignY(HSSParserNode::p value)
{
    this->dAlignY = value;
}

//defaults
void HSSDisplayObject::setDefaults()
{
    //width
    HSSPercentageConstant::p newDWidth(new HSSPercentageConstant(100.));
    this->setDWidth(newDWidth);
    //height
    HSSKeywordConstant::p newDHeight(new HSSKeywordConstant("content"));
    this->setDHeight(newDHeight);
    //anchorX
    HSSPercentageConstant::p newDAnchorX(new HSSPercentageConstant(50));
    this->setDAnchorX(newDAnchorX);
    //anchorY
    HSSPercentageConstant::p newDAnchorY(new HSSPercentageConstant(50));
    this->setDAnchorY(newDAnchorY);
    //flow
    HSSKeywordConstant::p newDFlow(new HSSKeywordConstant("yes"));
    this->setDFlow(newDFlow);
    //alignX
    HSSNumberConstant::p newDAlignX(new HSSNumberConstant(0));
    this->setDAlignX(newDAlignX);
    //alignY
    HSSNumberConstant::p newDAlignY(new HSSNumberConstant(0));
    this->setDAlignY(newDAlignY);
}
