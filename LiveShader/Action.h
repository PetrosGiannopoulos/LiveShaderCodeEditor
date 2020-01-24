#pragma once

class Action {

public:

	enum class ACTION_TYPE { CHAR_ADD, CHAR_DEL, PASTE_BLOCK };

	ACTION_TYPE actionType;
	glm::vec2 from;
	glm::vec2 to;

	vector<string> blockText;
public:

	Action() {}

	void setActionProperties(ACTION_TYPE actionType, glm::vec2 from, glm::vec2 to, vector<string> blockText) {
		this->actionType = actionType;
		this->from = from;
		this->to = to;
		this->blockText = blockText;
	}

	void setActionProperties(ACTION_TYPE actionType, glm::vec2 from, glm::vec2 to, char charText) {
		this->actionType = actionType;
		this->from = from;
		this->to = to;
		string charStr = "";
		charStr += charText;
		//cout << charStr << ": " << charText << endl;
		this->blockText.push_back(charStr);
	}
	
	ACTION_TYPE retrieveActionType() {
		return actionType;
	}
	
};